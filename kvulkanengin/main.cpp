#include <iostream>
#include <cassert>
#include <glfw3.h>

// Core
#include "core/context/kcorecontext.h"
#include "core/configs/instancedesc.h"
#include "core/configs/devicedesc.h"
#include "core/configs/swapchaindesc.h"

// Logic
#include "logic/context/klogiccontext.h"
#include "logic/renderer/forwardrenderer/kforwardrenderer.h"
#include "logic/platform/include/ikwindow.h"
#include "logic/platform/glfw/glfwwindow.h"

int main()
{
	// 1. App 层：使用原生 GLFW API 创建窗口
	if (!glfwInit())
	{
		std::cerr << "Failed to initialize GLFW!" << std::endl;
		return -1;
	}

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	GLFWwindow* glfwWindow = glfwCreateWindow(800, 600, "Vulkan Engine Test", nullptr, nullptr);
	if (!glfwWindow)
	{
		std::cerr << "Failed to create GLFW window!" << std::endl;
		glfwTerminate();
		return -1;
	}

	// 将原生 GLFW 窗口包装成 IKWindow 接口（logic 层使用）
	GlfwWindow window(glfwWindow);

	// 2. 创建 Core Context
	KCoreContext coreContext;

	KInstanceDesc instanceDesc;
	instanceDesc.applicationName = "Vulkan Engine Test";
	instanceDesc.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	instanceDesc.engineName = "KVulkanEngine";
	instanceDesc.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	instanceDesc.apiVersion = VK_API_VERSION_1_0;
	instanceDesc.benableValidation = true; // 测试时可以先关闭验证层

	// GLFW 需要的 Vulkan 扩展
	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
	for (uint32_t i = 0; i < glfwExtensionCount; ++i)
	{
		instanceDesc.extensions.push_back(glfwExtensions[i]);
	}

	if (!coreContext.CreateInstance(instanceDesc))
	{
		std::cerr << "Failed to create Vulkan instance!" << std::endl;
		glfwDestroyWindow(glfwWindow);
		glfwTerminate();
		return -1;
	}

	VkSurfaceKHR surface = window.CreateSurface(coreContext.GetInstance().GetHandle());
	if (surface == VK_NULL_HANDLE)
	{
		std::cerr << "Failed to create surface!" << std::endl;
		coreContext.Destroy();
		return -1;
	}
	coreContext.Attach(surface);

	KDeviceCreateDesc deviceDesc;
	deviceDesc._queueRequests.push_back({ QueueType::Graphics, 1, 1.0f });
	deviceDesc._queueRequests.push_back({ QueueType::Present, 1, 1.0f });
	deviceDesc._enabledFeatures._samplerAnisotropy = false; // 测试时可以先关闭
	deviceDesc._enabledExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

	if (!coreContext.CreateDevice(deviceDesc))
	{
		std::cerr << "Failed to create device!" << std::endl;
		coreContext.Destroy();
		return -1;
	}

	// 3. 创建 Logic Context
	KLogicContext logicContext;
	SwapchainCreateDesc swapchainDesc;
	swapchainDesc.width = window.GetWidth();
	swapchainDesc.height = window.GetHeight();
	swapchainDesc._desiredImageCount = 2;
	swapchainDesc._imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	if (!logicContext.Init(coreContext, swapchainDesc))
	{
		std::cerr << "Failed to initialize logic context!" << std::endl;
		coreContext.Destroy();
		return -1;
	}

	// 4. 创建 Renderer
	KForwardRenderer renderer;
	if (!renderer.Init(logicContext))
	{
		std::cerr << "Failed to initialize renderer!" << std::endl;
		logicContext.Destroy();
		coreContext.Destroy();
		return -1;
	}

	// 5. 设置 Renderer 和 Window
	logicContext.SetRenderer(&renderer);
	logicContext.SetWindow(&window);

	std::cout << "Initialization successful! Starting render loop..." << std::endl;

	// 6. 主循环
	while (!glfwWindowShouldClose(glfwWindow))
	{
		glfwPollEvents();

		// 开始一帧
		if (logicContext.BeginFrame())
		{
			// 渲染
			logicContext.Render();

			// 结束一帧
			logicContext.EndFrame();
		}
	}

	// 7. 清理资源
	std::cout << "Cleaning up..." << std::endl;
	
	// 等待设备空闲，确保所有命令执行完成（必须在销毁 renderer 之前）
	// framebuffer 和 renderpass 可能正在被命令缓冲区使用
	coreContext.GetDevice().WaitIdle();
	renderer.Destroy(); // 先销毁 renderer（framebuffer 和 renderpass）
	logicContext.Destroy(); // 然后销毁 logic context（包括 swapchain 和 frames）
	coreContext.Destroy(); // 最后销毁 core context（device, surface, instance）

	// App 层：清理原生 GLFW 资源
	glfwDestroyWindow(glfwWindow);
	glfwTerminate();

	std::cout << "Application exited successfully." << std::endl;
	return 0;
}
