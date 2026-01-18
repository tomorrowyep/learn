#ifndef __GLFW_WINDOW_H__
#define __GLFW_WINDOW_H__

#include "platform/include/ikwindow.h"
#include <glfw3.h>

class GlfwWindow : public IKWindow
{
public:
	// 从已存在的 GLFW 窗口创建（App 层使用）
	GlfwWindow(GLFWwindow* window) : m_window(window), m_ownsWindow(false) {}
	
	// 创建新窗口（logic 层内部使用，可选）
	GlfwWindow() : m_window(nullptr), m_ownsWindow(false) {}
	~GlfwWindow();

	// 初始化新窗口（logic 层内部使用）
	bool Init(uint32_t width, uint32_t height, const char* title);

	uint32_t GetWidth() const override;
	uint32_t GetHeight() const override;

	bool ShouldClose() const override;
	void PollEvents() override;

	VkSurfaceKHR CreateSurface(VkInstance instance) override;

	GLFWwindow* GetHandle() const { return m_window; }

private:
	GLFWwindow* m_window = nullptr;
	bool m_ownsWindow = false;  // 是否拥有窗口所有权（是否需要销毁）
};

#endif // !__GLFW_WINDOW_H__
