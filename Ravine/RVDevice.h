#ifndef RAVINE_DEVICE_H
#define RAVINE_DEVICE_H

//EASTL Includes
#include <eastl/vector.h>
#include <eastl/string.h>
using eastl::vector;
using eastl::string;

//Vulkan Includes
#include "volk.h"

//Ravine includes
#include "RvPersistentBuffer.h"
#include "RvDynamicBuffer.h"
#include "RvTexture.h"

class RvDevice
{
private:
	VkSurfaceKHR* surface;
	void createCommandPool();
	void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);

public:
	RvDevice(VkPhysicalDevice physicalDevice, VkSurfaceKHR& surface);
	~RvDevice();

	//Device Name
	string name;

	//Devices
	VkDevice handle;
	VkPhysicalDevice physicalDevice;

	//Queues
	VkQueue graphicsQueue;
	VkQueue presentQueue;
	//Default command pool for the graphics queue
	VkCommandPool commandPool = VK_NULL_HANDLE;

	//Physical Properties
	VkPhysicalDeviceMemoryProperties memProperties;
	VkPhysicalDeviceProperties deviceProperties;
	VkPhysicalDeviceFeatures supportedFeatures;

	//Attributes
	VkSampleCountFlagBits sampleCount;

	/**
	 * \brief Should be used instead of destroying in destructor
	 */
	void clear();

	RvTexture createTexture(void *pixels, size_t width, size_t height, VkFormat format = VK_FORMAT_R8G8B8A8_UNORM);
	void createImage(VkExtent3D extent, uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling,
		VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImageCreateFlagBits createFlagBits, VkImage& image, VkDeviceMemory& imageMemory) const; 
	
	RvDynamicBuffer createDynamicBuffer(VkDeviceSize bufferSize, VkBufferUsageFlagBits usageFlags, VkMemoryPropertyFlagBits memoryPropertyFlags);
	RvPersistentBuffer createPersistentBuffer(void* data, VkDeviceSize bufferSize, size_t sizeOfDataType, VkBufferUsageFlagBits usageFlags, 
		VkMemoryPropertyFlagBits memoryPropertyFlags);

	uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;
	VkFormat findSupportedFormat(const vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
	VkFormat findDepthFormat();
	VkSampleCountFlagBits getMaxUsableSampleCount();

	//Helper functions
	VkCommandBuffer beginSingleTimeCommands();
	void endSingleTimeCommands(VkCommandBuffer commandBuffer);

	VkFormatProperties getFormatProperties(VkFormat format);

};

#endif