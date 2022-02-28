#ifndef RV_WINDOW_H
#define RV_WINDOW_H

//GLFW include
#define VK_NO_PROTOTYPES
#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"

//Vulkan Include
#include "volk.h"

//STD Includes
#include "RvStdDefs.h"

//STD Includes
#include <stdexcept>

struct RvWindow
{
private:
	GLFWwindow * window;
	string title;
	VkInstance* instance;

public:
	RvWindow(uint32_t width, uint32_t height, const string title, bool fullscreen = false, GLFWframebuffersizefun resizeCallback = NULL);
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