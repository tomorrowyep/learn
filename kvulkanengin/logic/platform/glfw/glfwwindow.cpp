#include "glfwwindow.h"
#include <cassert>

GlfwWindow::~GlfwWindow()
{
	// 只有拥有窗口所有权时才销毁窗口
	if (m_ownsWindow && m_window)
	{
		glfwDestroyWindow(m_window);
		glfwTerminate();
		m_window = nullptr;
	}
}

bool GlfwWindow::Init(uint32_t width, uint32_t height, const char* title)
{
	if (!glfwInit())
		return false;

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	m_window = glfwCreateWindow(width, height, title, nullptr, nullptr);
	if (m_window)
	{
		m_ownsWindow = true;  // 标记拥有窗口所有权
		return true;
	}
	return false;
}

uint32_t GlfwWindow::GetWidth() const
{
	int w, h;
	glfwGetFramebufferSize(m_window, &w, &h);
	return static_cast<uint32_t>(w);
}

uint32_t GlfwWindow::GetHeight() const
{
	int w, h;
	glfwGetFramebufferSize(m_window, &w, &h);
	return static_cast<uint32_t>(h);
}

bool GlfwWindow::ShouldClose() const
{
	return glfwWindowShouldClose(m_window);
}

void GlfwWindow::PollEvents()
{
	glfwPollEvents();
}

VkSurfaceKHR GlfwWindow::CreateSurface(VkInstance instance)
{
	VkSurfaceKHR surface = VK_NULL_HANDLE;
	VkResult res = glfwCreateWindowSurface(instance, m_window, nullptr, &surface);
	assert(res == VK_SUCCESS);
	return surface;
}
