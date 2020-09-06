#include "RvSwapChain.h"

//EASTL Includes
#include <eastl/algorithm.h>

//STD Includes
#include <stdexcept>

RvSwapChain::RvSwapChain(RvDevice& device, VkSurfaceKHR surface, uint32_t width, uint32_t height, VkSwapchainKHR oldSwapChain)
{
	this->device = &device;
	this->surface = surface;
	this->width = width;
	this->height = height;

	//Check swap chain support
	RvSwapChainSupportDetails swapChainSupport = rvTools::querySupport(device.physicalDevice, surface);

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
	rvTools::QueueFamilyIndices indices = rvTools::findQueueFamilies(device.physicalDevice, surface);
	uint32_t queueFamilyIndices[] = {
		static_cast<uint32_t>(indices.graphicsFamily),
		static_cast<uint32_t>(indices.presentFamily)
	};

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

	//Pointer to old swapchain to enable speed-up things
	createInfo.oldSwapchain = oldSwapChain;

	//Create swap chain with given information
	if (vkCreateSwapchainKHR(device.handle, &createInfo, nullptr, &handle) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create swap chain!");
	}

	//Populate swap chain images and hold it's details
	vkGetSwapchainImagesKHR(device.handle, handle, &imageCount, nullptr);
	images.resize(imageCount);
	vkGetSwapchainImagesKHR(device.handle, handle, &imageCount, images.data());
	imageFormat = surfaceFormat.format;
	this->extent = extent;
}

VkSurfaceFormatKHR RvSwapChain::chooseSurfaceFormat(const vector<VkSurfaceFormatKHR>& availableFormats) {

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

VkPresentModeKHR RvSwapChain::choosePresentMode(const vector<VkPresentModeKHR>& availablePresentModes) {

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

	return bestMode;
}

VkExtent2D RvSwapChain::chooseExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
		return capabilities.currentExtent;
	}
	else {
		VkExtent2D actualExtent = { width, height };

		actualExtent.width = eastl::max(capabilities.minImageExtent.width, eastl::min(capabilities.maxImageExtent.width, actualExtent.width));
		actualExtent.height = eastl::max(capabilities.minImageExtent.height, eastl::min(capabilities.maxImageExtent.height, actualExtent.height));

		return actualExtent;
	}
}

RvSwapChain::operator VkSwapchainKHR() const
{
	return handle;
}

void RvSwapChain::clear()
{
	vkDestroySwapchainKHR(device->handle, handle, nullptr);

	//Destroy ImageViews
	for (VkImageView imageView : imageViews)
	{
		vkDestroyImageView(device->handle, imageView, nullptr);
	}

	destroySyncObjects();
}

void RvSwapChain::createImageViews()
{
	//Match the size
	imageViews.resize(images.size());
	for (size_t i = 0; i < images.size(); i++)
	{
		imageViews[i] = rvTools::createImageView(device->handle, images[i], imageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
	}
}

void RvSwapChain::createSyncObjects()
{
	imageAvailableSemaphores.resize(RV_MAX_FRAMES_IN_FLIGHT);
	renderFinishedSemaphores.resize(RV_MAX_FRAMES_IN_FLIGHT);
	inFlightFences.resize(RV_MAX_FRAMES_IN_FLIGHT);

	VkSemaphoreCreateInfo semaphoreInfo = {};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceInfo = {};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (size_t i = 0; i < RV_MAX_FRAMES_IN_FLIGHT; i++)
	{
		if (vkCreateSemaphore(device->handle, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
			vkCreateSemaphore(device->handle, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
			vkCreateFence(device->handle, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {

			throw std::runtime_error("Failed to create synchronization objects for a frame!");
		}
	}
}

void RvSwapChain::destroySyncObjects()
{
	for (size_t i = 0; i < RV_MAX_FRAMES_IN_FLIGHT; i++) {
		vkDestroySemaphore(device->handle, renderFinishedSemaphores[i], nullptr);
		vkDestroySemaphore(device->handle, imageAvailableSemaphores[i], nullptr);
		vkDestroyFence(device->handle, inFlightFences[i], nullptr);
	}
}

bool RvSwapChain::acquireNextFrame(uint32_t& frameIndex)
{
	//Wait for in-flight fences
	vkWaitForFences(device->handle, 1, &inFlightFences[currentFrame], VK_TRUE, eastl::numeric_limits<uint64_t>::max());
	vkResetFences(device->handle, 1, &inFlightFences[currentFrame]);

	//Acquiring an image from the swap chain
	//Reference: https://vulkan-tutorial.com/Drawing_a_triangle/Drawing/Rendering_and_presentation#page_Acquiring_an_image_from_the_swap_chain
	const VkResult result = vkAcquireNextImageKHR(device->handle, handle, eastl::numeric_limits<uint64_t>::max(),
	                                        imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &frameIndex);
	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		return false;
	}
	if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
		throw std::runtime_error("Failed to acquire swap chain image!");
	}

	return true;
}

bool RvSwapChain::submitNextFrame(VkCommandBuffer* commandBuffers, uint32_t frameIndex)
{
	//Submitting the command queue
	//Reference: https://vulkan-tutorial.com/Drawing_a_triangle/Drawing/Rendering_and_presentation#page_Submitting_the_command_buffer
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore waitSemaphores[] = { imageAvailableSemaphores[currentFrame] };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;

	//TODO: Research on multiple command buffers for same frame
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffers[frameIndex];

	VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[currentFrame] };
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;
	
	VkResult result = vkQueueSubmit(device->graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to submit draw command buffer!");
	}

	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;

	VkSwapchainKHR swapChains[] = { handle };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &frameIndex;

	presentInfo.pResults = nullptr; // Optional

	result = vkQueuePresentKHR(device->presentQueue, &presentInfo);
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
		framebufferResized = false;
		return false;
	}
	if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to present swap chain image!");
	}

	currentFrame = (currentFrame + 1) % RV_MAX_FRAMES_IN_FLIGHT;
	return true;
}

RvSwapChain::~RvSwapChain()
= default;
