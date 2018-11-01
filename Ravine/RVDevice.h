#pragma once

//STD Includes
#include <vector>

//Vulkan Includes
#include <vulkan\vulkan.h>

//Ravine includes
#include "RvPersistentBuffer.h"

class RvDevice
{
private:
	VkSurfaceKHR* surface;
	void CreateCommandPool();

	void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);

public:
	RvDevice(VkPhysicalDevice physicalDevice, VkSurfaceKHR& surface);
	~RvDevice();

	//Devices
	VkDevice handle;
	VkPhysicalDevice physicalDevice;

	//Queues
	VkQueue graphicsQueue;
	VkQueue presentQueue;
	// Default command pool for the graphics queue
	VkCommandPool commandPool = VK_NULL_HANDLE;

	//Physical Properties
	VkPhysicalDeviceMemoryProperties memProperties;
	VkPhysicalDeviceProperties deviceProperties;
	VkPhysicalDeviceFeatures supportedFeatures;

	//Attributes
	VkSampleCountFlagBits sampleCount;

	//Should be used instead of destroying in destructor
	void Clear();

	void createImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage & image, VkDeviceMemory & imageMemory);
	RvPersistentBuffer createPersistentBuffer(void* data, size_t bufferSize, size_t sizeOfDataType);

	uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
	VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
	VkFormat findDepthFormat();
	VkSampleCountFlagBits getMaxUsableSampleCount();

	//Helper functions
	VkCommandBuffer beginSingleTimeCommands();
	void endSingleTimeCommands(VkCommandBuffer commandBuffer);

	VkFormatProperties getFormatProperties(VkFormat format);

	operator VkDevice() {
		return handle;
	}

};

