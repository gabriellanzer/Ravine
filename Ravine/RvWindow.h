#ifndef RV_WINDOW_H
#define RV_WINDOW_H

//GLFW include
#define GLFW_INCLUDE_VULKAN
#include <glfw/glfw3.h>

//Vulkan Include
#include <vulkan/vulkan.h>

//STD Includes
#include <string>

struct RvWindow
{
private:
	GLFWwindow * window;
	std::string title;
	VkInstance* instance;

public:
	RvWindow(uint32_t width, uint32_t height, const std::string title, bool fullscreen = false, GLFWframebuffersizefun resizeCallback = NULL);
	~RvWindow();
	
	VkSurfaceKHR surface;
	VkExtent2D extent;
	bool framebufferResized = false;

	void CreateSurface(VkInstance& instance);

	operator GLFWwindow*() {
		return window;
	}
};

#endif