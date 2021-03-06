#ifndef RV_SWAPCHAIN_H
#define RV_SWAPCHAIN_H

//EASTL Includes
#include <eastl/vector.h>
using eastl::vector;

//Vulkan Includes
#include "volk.h"

//Ravine Includes
#include "RvTools.h"
#include "RvDevice.h"

//Forward declaration of Swap Chain Support details struct
struct RvSwapChainSupportDetails;

struct RvSwapChain
{
private:
	//External Dependencies
	RvDevice* device;
	VkSurfaceKHR surface;

	size_t currentFrame = 0;

public:
	//Extent values
	uint32_t width;
	uint32_t height;
	bool framebufferResized = false;

	//Number of maximum simultaneous frames
	#define RV_MAX_FRAMES_IN_FLIGHT 3

	//Vulkan Object Handle
	VkSwapchainKHR handle = VK_NULL_HANDLE;

	//Swap chain properties
	vector<VkImage> images;
	VkFormat imageFormat;
	VkExtent2D extent;
	vector<VkImageView> imageViews;

	//Queue fences
	vector<VkFence> inFlightFences;

	//Queues semaphores
	vector<VkSemaphore> imageAvailableSemaphores;
	vector<VkSemaphore> renderFinishedSemaphores;

	RvSwapChain(RvDevice& device, VkSurfaceKHR surface, uint32_t width, uint32_t height, VkSwapchainKHR oldSwapChain);
	~RvSwapChain();

	void clear();

	void createImageViews();

	void createSyncObjects();
	void destroySyncObjects();

	bool acquireNextFrame(uint32_t& frameIndex);
	bool submitNextFrame(VkCommandBuffer* commandBuffers, uint32_t frameIndex);

	VkSurfaceFormatKHR chooseSurfaceFormat(const vector<VkSurfaceFormatKHR>& availableFormats);
	VkPresentModeKHR choosePresentMode(const vector<VkPresentModeKHR>& availablePresentModes);
	VkExtent2D chooseExtent(const VkSurfaceCapabilitiesKHR& capabilities);

	explicit operator VkSwapchainKHR() const;
};

struct RvSwapChainSupportDetails
{
	VkSurfaceCapabilitiesKHR capabilities;
	vector<VkSurfaceFormatKHR> formats;
	vector<VkPresentModeKHR> presentModes;
};

#endif
