#pragma once

#include <vulkan\vulkan.h>
#include <vector>

class RvDevice
{
public:
	RvDevice(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);
	~RvDevice();

	//Devices
	VkDevice handle;
	VkPhysicalDevice physicalDevice;

	//Queues
	VkQueue graphicsQueue;
	VkQueue presentQueue;

	//Physical Properties
	VkPhysicalDeviceMemoryProperties memProperties;
	VkPhysicalDeviceProperties deviceProperties;
	VkPhysicalDeviceFeatures supportedFeatures;

	//Attributes
	VkSampleCountFlagBits sampleCount;

	void createImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage & image, VkDeviceMemory & imageMemory);

	uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
	VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
	VkFormat findDepthFormat();
	VkSampleCountFlagBits getMaxUsableSampleCount();

	VkFormatProperties getFormatProperties(VkFormat format);

	operator VkDevice() {
		return handle;
	}

};

