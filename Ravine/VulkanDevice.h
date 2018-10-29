#pragma once

#include <vulkan\vulkan.h>
#include <vector>

class VulkanDevice
{
public:
	VulkanDevice();
	~VulkanDevice();

	VkDevice device;
	VkPhysicalDevice physicalDevice;

	VkPhysicalDeviceMemoryProperties memProperties;
	VkPhysicalDeviceProperties deviceProperties;
	VkPhysicalDeviceFeatures supportedFeatures;

	uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
	VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
	VkSampleCountFlagBits getMaxUsableSampleCount();

};

