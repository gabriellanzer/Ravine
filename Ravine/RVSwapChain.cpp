#include "RVSwapChain.h"

//Local dependencies
#include <algorithm>


RvSwapChain::RvSwapChain(VkDevice& device, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, uint32_t WIDTH, uint32_t HEIGHT, VkSwapchainKHR oldSwapChain)
{
	//TODO: Use new constructor thingy
	this->device = &device;
	this->surface = surface;
	this->WIDTH = WIDTH;
	this->HEIGHT = HEIGHT;

	//Check swap chain support
	SwapChainSupportDetails swapChainSupport = vkTools::querySupport(physicalDevice, surface);

	//Define swap chain setup data
	VkSurfaceFormatKHR surfaceFormat = chooseSurfaceFormat(swapChainSupport.formats);
	VkPresentModeKHR presentMode = choosePresentMode(swapChainSupport.presentModes);
	VkExtent2D extent = chooseExtent(swapChainSupport.capabilities);

	//Define amount of images in swap chain
	uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
	if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
		imageCount = swapChainSupport.capabilities.maxImageCount;
	}

	//Create Structure
	VkSwapchainCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = surface;

	//Swap chain image details
	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	//Define queue sharing modes based on ids comparison
	vkTools::QueueFamilyIndices indices = vkTools::findQueueFamilies(physicalDevice, surface);
	uint32_t queueFamilyIndices[] = { (uint32_t)indices.graphicsFamily, (uint32_t)indices.presentFamily };

	if (indices.graphicsFamily != indices.presentFamily) {
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	else {
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.queueFamilyIndexCount = 0; // Optional
		createInfo.pQueueFamilyIndices = nullptr; // Optional
	}

	//Ignore alpha channel when blending with other windows
	createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

	//Define swap present mode and make sure we ignore windows pixels in front of ours
	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;

	//TODO: Change that to create another swap chain when resize screen
	createInfo.oldSwapchain = oldSwapChain;

	//Create swap chain with given information
	if (vkCreateSwapchainKHR(*this->device, &createInfo, nullptr, &handle) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create swap chain!");
	}

	//Populate swap chain images and hold it's details
	vkGetSwapchainImagesKHR(*this->device, handle, &imageCount, nullptr);
	images.resize(imageCount);
	vkGetSwapchainImagesKHR(*this->device, handle, &imageCount, images.data());
	imageFormat = surfaceFormat.format;
	extent = extent;
}

VkSurfaceFormatKHR RvSwapChain::chooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {

	if (availableFormats.size() == 1 && availableFormats[0].format == VK_FORMAT_UNDEFINED) {
		return { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
	}

	for (const auto& availableFormat : availableFormats) {
		if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			return availableFormat;
		}
	}

	return availableFormats[0];
}

VkPresentModeKHR RvSwapChain::choosePresentMode(const std::vector<VkPresentModeKHR> availablePresentModes) {

	//FIFO v-sync might not be available on some drivers
	VkPresentModeKHR bestMode = VK_PRESENT_MODE_FIFO_KHR;

	for (const auto& availablePresentMode : availablePresentModes) {
		//Check if triple-buffering is available (low latency v-sync)
		if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
			return availablePresentMode;
		}
		//Better to use NO V-Sync to avoid unavailable modes
		else if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
			bestMode = availablePresentMode;
		}
	}

	return VK_PRESENT_MODE_IMMEDIATE_KHR;
}

VkExtent2D RvSwapChain::chooseExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
		return capabilities.currentExtent;
	}
	else {
		VkExtent2D actualExtent = { WIDTH, HEIGHT };

		actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
		actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));

		return actualExtent;
	}
}

void RvSwapChain::Clear()
{
	for (VkImageView imageView : imageViews) {
		vkDestroyImageView(*device, imageView, nullptr);
	}
	vkDestroySwapchainKHR(*device, handle, nullptr);
}

void RvSwapChain::CreateImageViews()
{
	//Match the size
	imageViews.resize(images.size());
	for (size_t i = 0; i < images.size(); i++)
	{
		imageViews[i] = vkTools::createImageView(*device, images[i], imageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
	}
}

void RvSwapChain::CreateSyncObjects()
{
	imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

	VkSemaphoreCreateInfo semaphoreInfo = {};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceInfo = {};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		if (vkCreateSemaphore(*device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
			vkCreateSemaphore(*device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
			vkCreateFence(*device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {

			throw std::runtime_error("Failed to create synchronization objects for a frame!");
		}
	}
}

void RvSwapChain::DestroySyncObjects()
{
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		vkDestroySemaphore(*device, renderFinishedSemaphores[i], nullptr);
		vkDestroySemaphore(*device, imageAvailableSemaphores[i], nullptr);
		vkDestroyFence(*device, inFlightFences[i], nullptr);
	}
}

RvSwapChain::~RvSwapChain()
{
	vkDestroySwapchainKHR(*device, *this, nullptr);
}
