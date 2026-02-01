#include "engine.h"
#include <GLFW/glfw3.h>
#include <iostream>
#include <chrono>

namespace kEngine
{

Engine::~Engine()
{
	shutdown();
}

bool Engine::init(const EngineConfig& config)
{
	m_config = config;

	if (!initWindow())
		return false;

	if (!initVulkan())
		return false;

	if (!initFrameData())
		return false;

	m_resources.init(&m_coreContext);

	if (!m_forwardPass.init(m_coreContext, m_swapchain, m_resources))
	{
		std::cerr << "[Engine] ForwardPass init failed" << std::endl;
		return false;
	}

	m_initialized = true;
	std::cout << "[Engine] Initialized successfully" << std::endl;
	return true;
}

void Engine::shutdown()
{
	if (!m_initialized)
		return;

	m_coreContext.GetDevice().WaitIdle();
	m_forwardPass.destroy();
	m_resources.destroy();
	destroyFrameData();
	m_swapchain.Destroy();
	m_coreContext.Destroy();

	if (m_window)
	{
		glfwDestroyWindow(m_window);
		glfwTerminate();
		m_window = nullptr;
	}

	m_initialized = false;
	std::cout << "[Engine] Shutdown" << std::endl;
}

void Engine::run(std::function<void(float)> updateCallback)
{
	auto lastTime = std::chrono::high_resolution_clock::now();

	while (!glfwWindowShouldClose(m_window))
	{
		glfwPollEvents();

		auto currentTime = std::chrono::high_resolution_clock::now();
		m_deltaTime = std::chrono::duration<float>(currentTime - lastTime).count();
		lastTime = currentTime;
		m_totalTime += m_deltaTime;

		if (updateCallback)
			updateCallback(m_deltaTime);

		SceneRenderer::extract(m_scene, m_renderScene, aspectRatio());

		uint32_t imageIndex;
		if (beginFrame(imageIndex))
		{
			render(imageIndex);
			endFrame(imageIndex);
		}
	}

	m_coreContext.GetDevice().WaitIdle();
}

bool Engine::initWindow()
{
	if (!glfwInit())
	{
		std::cerr << "[Engine] Failed to init GLFW" << std::endl;
		return false;
	}

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

	m_window = glfwCreateWindow(m_config.width, m_config.height, m_config.title.c_str(), nullptr, nullptr);
	if (!m_window)
	{
		std::cerr << "[Engine] Failed to create window" << std::endl;
		return false;
	}

	glfwSetWindowUserPointer(m_window, this);
	glfwSetFramebufferSizeCallback(m_window, [](GLFWwindow* win, int w, int h)
	{
		auto* engine = static_cast<Engine*>(glfwGetWindowUserPointer(win));
		engine->m_framebufferResized = true;
		engine->m_config.width = w;
		engine->m_config.height = h;
	});

	return true;
}

bool Engine::initVulkan()
{
	// Get GLFW extensions
	uint32_t glfwExtCount = 0;
	const char** glfwExts = glfwGetRequiredInstanceExtensions(&glfwExtCount);
	std::vector<const char*> extensions(glfwExts, glfwExts + glfwExtCount);

	// Create instance
	KInstanceDesc instanceDesc;
	instanceDesc.applicationName = m_config.title.c_str();
	instanceDesc.benableValidation = m_config.validation;
	instanceDesc.extensions = extensions;

	if (!m_coreContext.CreateInstance(instanceDesc))
	{
		std::cerr << "[Engine] CreateInstance failed" << std::endl;
		return false;
	}

	// Create surface
	VkSurfaceKHR surface;
	if (glfwCreateWindowSurface(m_coreContext.GetInstance().GetHandle(), m_window, nullptr, &surface) != VK_SUCCESS)
	{
		std::cerr << "[Engine] Failed to create window surface" << std::endl;
		return false;
	}
	m_coreContext.Attach(surface);

	// Create device
	KDeviceCreateDesc deviceDesc;
	deviceDesc._queueRequests.push_back({QueueType::Graphics, 1, 1.0f});
	deviceDesc._queueRequests.push_back({QueueType::Present, 1, 1.0f});
	deviceDesc._enabledFeatures._samplerAnisotropy = true;
	deviceDesc._enabledExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

	if (!m_coreContext.CreateDevice(deviceDesc))
	{
		std::cerr << "[Engine] CreateDevice failed" << std::endl;
		return false;
	}

	// Create swapchain
	SwapChainSupportDetails swapSupport;
	m_coreContext.GetPhysicalDevice().QuerySwapChainSupport(m_coreContext.GetSurface().GetHandle(), swapSupport);

	SwapchainCreateDesc swapDesc;
	swapDesc.width = m_config.width;
	swapDesc.height = m_config.height;
	if (!m_config.vsync)
		swapDesc._preferredPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;

	if (!m_swapchain.Create(m_coreContext.GetDevice(), m_coreContext.GetSurface().GetHandle(), swapSupport, swapDesc))
	{
		std::cerr << "[Engine] Swapchain create failed" << std::endl;
		return false;
	}

	return true;
}

bool Engine::initFrameData()
{
	m_frames.resize(MAX_FRAMES);

	VkDevice device = m_coreContext.GetDevice().GetHandle();
	uint32_t queueFamily = m_coreContext.GetDevice().GetQueueFamilies()._graphicsFamily;

	for (auto& frame : m_frames)
	{
		frame.commandPool.Init(device, queueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
		frame.commandBuffer.Allocate(device, frame.commandPool.GetHandle());
		frame.imageAvailable.Init(device);
		frame.renderFinished.Init(device);
		frame.inFlight.Init(device, true);
	}

	return true;
}

void Engine::destroyFrameData()
{
	for (auto& frame : m_frames)
	{
		frame.inFlight.Destroy();
		frame.renderFinished.Destroy();
		frame.imageAvailable.Destroy();
		frame.commandBuffer.Free(frame.commandPool.GetHandle());
		frame.commandPool.Destroy();
	}
	m_frames.clear();
}

bool Engine::beginFrame(uint32_t& imageIndex)
{
	auto& frame = m_frames[m_currentFrame];

	frame.inFlight.Wait();

	VkResult result = m_swapchain.AcquireNextImage(*frame.imageAvailable.GetHandle(), &imageIndex);
	if (result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		handleResize();
		return false;
	}

	frame.inFlight.Reset();
	frame.commandBuffer.Reset();
	frame.commandBuffer.Begin();

	return true;
}

void Engine::render(uint32_t imageIndex)
{
	auto& frame = m_frames[m_currentFrame];
	m_forwardPass.render(frame.commandBuffer.GetHandle(), imageIndex, m_renderScene, m_resources);
}

void Engine::endFrame(uint32_t imageIndex)
{
	auto& frame = m_frames[m_currentFrame];

	frame.commandBuffer.End();

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore waitSemaphores[] = {*frame.imageAvailable.GetHandle()};
	VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;

	VkCommandBuffer cmdBuf = frame.commandBuffer.GetHandle();
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cmdBuf;

	VkSemaphore signalSemaphores[] = {*frame.renderFinished.GetHandle()};
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	m_coreContext.GetDevice().SubmitGraphics(submitInfo, frame.inFlight.GetHandle());

	VkResult result = m_swapchain.Present(*frame.renderFinished.GetHandle(), imageIndex);
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_framebufferResized)
	{
		m_framebufferResized = false;
		handleResize();
	}

	m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES;
}

void Engine::handleResize()
{
	int width = 0, height = 0;
	glfwGetFramebufferSize(m_window, &width, &height);

	while (width == 0 || height == 0)
	{
		glfwGetFramebufferSize(m_window, &width, &height);
		glfwWaitEvents();
	}

	m_coreContext.GetDevice().WaitIdle();

	m_config.width = width;
	m_config.height = height;

	SwapChainSupportDetails swapSupport;
	m_coreContext.GetPhysicalDevice().QuerySwapChainSupport(m_coreContext.GetSurface().GetHandle(), swapSupport);

	SwapchainCreateDesc swapDesc;
	swapDesc.width = width;
	swapDesc.height = height;

	m_swapchain.Recreate(m_coreContext.GetDevice(), m_coreContext.GetSurface().GetHandle(), swapSupport, swapDesc);
	m_forwardPass.onResize(m_coreContext, m_swapchain);
}

} // namespace kEngine
