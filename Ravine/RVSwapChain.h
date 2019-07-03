#ifndef RV_SWAPCHAIN_H
#define RV_SWAPCHAIN_H

//EASTL Includes
#include <eastl/vector.h>
using eastl::vector;
#include "EASTL_new.h"

//Vulkan Includes
#include <vulkan/vulkan.h>

//Ravine Includes
#include "RvTools.h"
#include "RvDevice.h"
#include "RvFramebufferAttachment.h"

//Forward declaration of Swap Chain Support details struct
struct SwapChainSupportDetails;

struct RvSwapChain
{
private:
	//External Dependencies
	RvDevice* device;
	VkSurfaceKHR surface;

	size_t currentFrame = 0;

public:
	//Extent values
	uint32_t WIDTH;
	uint32_t HEIGHT;
	bool framebufferResized = false;

	//Number of maximum simultaneous frames
	#define RV_MAX_FRAMES_IN_FLIGHT 3

	//Vulkan Object Handle
	VkSwapchainKHR handle = VK_NULL_HANDLE;

	//Renderpass
	VkRenderPass renderPass;

	//Swap chain properties
	vector<VkImage> images;
	VkFormat imageFormat;
	VkExtent2D extent;
	vector<VkImageView> imageViews;

	//Framebuffers
	vector<VkFramebuffer> framebuffers;
	vector<RvFramebufferAttachment> framebufferAttachments;

	//Queue fences
	vector<VkFence> inFlightFences;

	//Queues semaphors
	vector<VkSemaphore> imageAvailableSemaphores;
	vector<VkSemaphore> renderFinishedSemaphores;

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


	VkSurfaceFormatKHR chooseSurfaceFormat(const vector<VkSurfaceFormatKHR>& availableFormats);

	VkPresentModeKHR choosePresentMode(const vector<VkPresentModeKHR> availablePresentModes);

	VkExtent2D chooseExtent(const VkSurfaceCapabilitiesKHR& capabilities);

	operator VkSwapchainKHR()
	{
		return handle;
	}
};

struct SwapChainSupportDetails
{
	VkSurfaceCapabilitiesKHR capabilities;
	vector<VkSurfaceFormatKHR> formats;
	vector<VkPresentModeKHR> presentModes;
};

#endif