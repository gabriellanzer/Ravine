#pragma once

//Vulkan Includes
#include <vulkan\vulkan.h>

//STD Includes
#include <vector>

//Ravine Includes
#include "RvSwapChain.h"
#include "RvDevice.h"

struct SwapChainSupportDetails;

namespace vkTools
{

	bool hasStencilComponent(VkFormat format);

	VkImageView createImageView(VkDevice device, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels = 0);

	//Structure for Queue Family query of available queue types
	//TODO: Move to Device
	struct QueueFamilyIndices {
		int graphicsFamily = -1;
		int presentFamily = -1;

		//Is this struct complete to be used?
		bool isComplete() {
			return graphicsFamily >= 0 && presentFamily >= 0;
		}
	};

	//TODO: Move to Device
	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface);

	SwapChainSupportDetails querySupport(VkPhysicalDevice device, VkSurfaceKHR surface);

	std::vector<char> readFile(const std::string& filename);

	void transitionImageLayout(RvDevice device, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels);

};

