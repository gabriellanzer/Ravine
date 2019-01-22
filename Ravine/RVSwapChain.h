
#ifndef RV_SWAPCHAIN_H
#define RV_SWAPCHAIN_H

//STD Includes
#include <vector>

//Vulkan Includes
#include <vulkan/vulkan.h>

//Ravine Includes
#include "RvTools.h"
#include "RvDevice.h"
#include "RvFramebufferAttachment.h"

//Structure for Swap Chain Support query of details
struct SwapChainSupportDetails {
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};

struct RvSwapChain
{
private:
	//External Dependencies
	RvDevice* device;
	VkSurfaceKHR surface;

	size_t currentFrame = 0;

	bool framebufferResized = false;
public:
	//Extent values
	uint32_t WIDTH;
	uint32_t HEIGHT;

	//Number of maximum simultaneous frames
	#define RV_MAX_FRAMES_IN_FLIGHT 3

	//Vulkan Object Handle
	VkSwapchainKHR handle = VK_NULL_HANDLE;

	//Renderpass
	VkRenderPass renderPass;

	//Swap chain properties
	std::vector<VkImage> images;
	VkFormat imageFormat;
	VkExtent2D extent;
	std::vector<VkImageView> imageViews;

	//Framebuffers
	std::vector<VkFramebuffer> framebuffers;
	std::vector<RvFramebufferAttachment> framebufferAttachments;

	//Queue fences
	std::vector<VkFence> inFlightFences;

	//Queues semaphors
	std::vector<VkSemaphore> imageAvailableSemaphores;
	std::vector<VkSemaphore> renderFinishedSemaphores;

	RvSwapChain(RvDevice& device, VkSurfaceKHR surface, uint32_t WIDTH, uint32_t HEIGHT, VkSwapchainKHR oldSwapChain);
	~RvSwapChain();

	void Clear();

	void CreateImageViews();
	void CreateRenderPass();
	void AddFramebufferAttachment(RvFramebufferAttachmentCreateInfo createInfo);
	void CreateFramebuffers();

	void CreateSyncObjects();

	void DestroySyncObjects();

	bool AcquireNextFrame(uint32_t& frameIndex);
	bool SubmitNextFrame(VkCommandBuffer* commandBuffers, uint32_t frameIndex);


	VkSurfaceFormatKHR chooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);

	VkPresentModeKHR choosePresentMode(const std::vector<VkPresentModeKHR> availablePresentModes);

	VkExtent2D chooseExtent(const VkSurfaceCapabilitiesKHR& capabilities);

	operator VkSwapchainKHR()
	{
		return handle;
	}
};

#endif
