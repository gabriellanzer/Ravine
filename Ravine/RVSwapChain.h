
#ifndef RV_SWAPCHAIN_H
#define RV_SWAPCHAIN_H

//STD Includes
#include <vector>

//Vulkan Includes
#include <vulkan/vulkan.h>

//Vulkan Tools
#include "VulkanTools.h"

//VK Wrappers
#include "RVDevice.h"

//Structure for Swap Chain Support query of details
struct SwapChainSupportDetails {
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};

struct RvSwapChain
{
	//External Dependencies
	RvDevice* device;
	VkSurfaceKHR surface;

	//Extent values
	uint32_t WIDTH;
	uint32_t HEIGHT;

	//Number of maximum simultaneous frames
	const int MAX_FRAMES_IN_FLIGHT = 2;

	//Vulkan Object Handle
	VkSwapchainKHR handle = VK_NULL_HANDLE;

	//Swap chain properties
	std::vector<VkImage> images;
	VkFormat imageFormat;
	VkExtent2D extent;
	std::vector<VkImageView> imageViews;

	//Queue fences
	std::vector<VkFence> inFlightFences;

	//Queues semaphors
	std::vector<VkSemaphore> imageAvailableSemaphores;
	std::vector<VkSemaphore> renderFinishedSemaphores;

	RvSwapChain(RvDevice& device, VkSurfaceKHR surface, uint32_t WIDTH, uint32_t HEIGHT, VkSwapchainKHR oldSwapChain);

	void Clear();

	void CreateImageViews();

	void CreateSyncObjects();

	void DestroySyncObjects();

	~RvSwapChain();

	VkSurfaceFormatKHR chooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);

	VkPresentModeKHR choosePresentMode(const std::vector<VkPresentModeKHR> availablePresentModes);

	VkExtent2D chooseExtent(const VkSurfaceCapabilitiesKHR& capabilities);

	operator VkSwapchainKHR()
	{
		return handle;
	}
};

#endif
