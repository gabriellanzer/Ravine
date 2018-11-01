#include "RvWindow.h"


RvWindow::RvWindow(uint32_t width, uint32_t height, const std::string title, bool fullscreen, GLFWframebuffersizefun resizeCallback) :
	extent({ width, height }), title(title)
{
	if (resizeCallback != NULL)
	{
		glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
	}

	window = glfwCreateWindow(width, height, title.c_str(),
		(fullscreen) ? glfwGetPrimaryMonitor() : nullptr,
		nullptr);

	//Storing Ravine pointer inside window instance
	glfwSetWindowUserPointer(window, this);

	if (resizeCallback != NULL)
	{
		glfwSetFramebufferSizeCallback(window, resizeCallback);
	}
}


RvWindow::~RvWindow()
{
	vkDestroySurfaceKHR(*instance, surface, nullptr);
	glfwDestroyWindow(window);
}

void RvWindow::CreateSurface(VkInstance& instance)
{
	this->instance = &instance;
	if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create window surface!");
	}
}
