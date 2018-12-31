#include "RVDevice.h"

//STD Includes
#include <algorithm>
#include <set>
#include <iostream>

//Ravine System Includes
#include "RvTools.h"
#include "RVConfig.h"

RvDevice::RvDevice(VkPhysicalDevice physicalDevice, VkSurfaceKHR& surface) : physicalDevice(physicalDevice), surface(&surface)
{
	rvTools::QueueFamilyIndices indices = rvTools::findQueueFamilies(physicalDevice, surface);

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<int> uniqueQueueFamilies = { indices.graphicsFamily, indices.presentFamily };

	float queuePriority = 1.0f;
	for (int queueFamily : uniqueQueueFamilies) {
		VkDeviceQueueCreateInfo queueCreateInfo = {};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.push_back(queueCreateInfo);
	}

	VkPhysicalDeviceFeatures deviceFeatures = {};
	deviceFeatures.samplerAnisotropy = VK_TRUE; //Enabling anisotropy
	deviceFeatures.sampleRateShading = VK_TRUE; //Enable sample shading feature for the device

	VkDeviceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

	createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
	createInfo.pQueueCreateInfos = queueCreateInfos.data();

	createInfo.pEnabledFeatures = &deviceFeatures;

	createInfo.enabledExtensionCount = static_cast<uint32_t>(rvCfg::DeviceExtensions.size());
	createInfo.ppEnabledExtensionNames = rvCfg::DeviceExtensions.data();

#ifdef VALIDATION_LAYERS_ENABLED
		createInfo.enabledLayerCount = static_cast<uint32_t>(rvCfg::ValidationLayers.size());
		createInfo.ppEnabledLayerNames = rvCfg::ValidationLayers.data();
#else
		createInfo.enabledLayerCount = 0;
#endif

	if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &handle) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create logical device!");
	}

	vkGetDeviceQueue(handle, indices.graphicsFamily, 0, &graphicsQueue);
	vkGetDeviceQueue(handle, indices.presentFamily, 0, &presentQueue);

	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);
	vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);
	vkGetPhysicalDeviceFeatures(physicalDevice, &supportedFeatures);

	//TODO: This should be dynamically chosen
	sampleCount = getMaxUsableSampleCount();
	std::cout << "Choosen samples count: " << sampleCount << std::endl;

	//Create command pool
	CreateCommandPool();
}


RvDevice::~RvDevice()
{
}

void RvDevice::CreateCommandPool()
{
	rvTools::QueueFamilyIndices queueFamilyIndices = rvTools::findQueueFamilies(physicalDevice, *surface);

	VkCommandPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily;
	poolInfo.flags = 0; // Optional

	if (vkCreateCommandPool(handle, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create command pool!");
	}
}

void RvDevice::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer & buffer, VkDeviceMemory & bufferMemory)
{
	//Defining buffer creation info
	VkBufferCreateInfo bufferCreateInfo = {};
	bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferCreateInfo.size = size;
	bufferCreateInfo.usage = usage;
	bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	//Creating buffer
	if (vkCreateBuffer(handle, &bufferCreateInfo, nullptr, &buffer) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create buffer!");
	}

	//Allocating buffer memory
	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(handle, buffer, &memRequirements);

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

	if (vkAllocateMemory(handle, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
		throw std::runtime_error("Failed to allocate buffer memory!");
	}

	//Binding buffer memory
	vkBindBufferMemory(handle, buffer, bufferMemory, 0);
}

void RvDevice::Clear()
{
	//Destroy command pool actually destroys all CMD Buffers
	vkDestroyCommandPool(handle, commandPool, nullptr);

	//Cleanup all device data
	vkDestroyDevice(handle, nullptr);
}

void RvDevice::createImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImageCreateFlagBits createFlagBits, VkImage & image, VkDeviceMemory & imageMemory)
{
	//Defining image creation info
	VkImageCreateInfo imageCreateInfo = {};
	imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
	imageCreateInfo.extent.width = width;
	imageCreateInfo.extent.height = height;
	imageCreateInfo.extent.depth = 1;
	imageCreateInfo.mipLevels = mipLevels;
	imageCreateInfo.arrayLayers = 1;
	imageCreateInfo.format = format;
	imageCreateInfo.tiling = tiling;
	imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageCreateInfo.usage = usage;
	imageCreateInfo.samples = numSamples;
	imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageCreateInfo.flags = createFlagBits;

	//Creating image
	if (vkCreateImage(handle, &imageCreateInfo, nullptr, &image) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create image!");
	}

	//Allocating image memory
	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(handle, image, &memRequirements);

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

	if (vkAllocateMemory(handle, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
		throw std::runtime_error("Failed to allocate image memory!");
	}

	//Binding image and memory
	vkBindImageMemory(handle, image, imageMemory, 0);
}

RvDynamicBuffer RvDevice::createDynamicBuffer(VkDeviceSize bufferSize, VkBufferUsageFlagBits usageFlags, VkMemoryPropertyFlagBits memoryProperyFlags)
{
	RvDynamicBuffer newBuffer;
	createBuffer(bufferSize, usageFlags, memoryProperyFlags, newBuffer.buffer, newBuffer.memory);
	return newBuffer;
}

RvPersistentBuffer RvDevice::createPersistentBuffer(void * data, VkDeviceSize bufferSize, size_t sizeOfDataType, VkBufferUsageFlagBits usageFlags, VkMemoryPropertyFlagBits memoryProperyFlags)
{
	// Staging Buffer
	RvDynamicBuffer stagingBuffer = createDynamicBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, (VkMemoryPropertyFlagBits)(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT));

	// Copying data
	void* stagingData;
	vkMapMemory(handle, stagingBuffer.memory, 0, bufferSize, 0, &stagingData);
	memcpy(stagingData, (void*)data, (size_t)bufferSize);
	vkUnmapMemory(handle, stagingBuffer.memory);

	// Persistent buffer
	RvPersistentBuffer newBuffer(bufferSize, sizeOfDataType);
	createBuffer(bufferSize, usageFlags, memoryProperyFlags, newBuffer.handle, newBuffer.memory);

	// Copying data to persistent buffer
	rvTools::copyBuffer(*this, stagingBuffer.buffer, newBuffer.handle, bufferSize);

	//Clearing staging buffer
	vkDestroyBuffer(handle, stagingBuffer.buffer, nullptr);
	vkFreeMemory(handle, stagingBuffer.memory, nullptr);

	return newBuffer;
}

RvTexture RvDevice::createTexture(void* pixels, size_t width, size_t height, VkFormat format = VK_FORMAT_R8G8B8A8_UNORM)
{
	return rvTools::createTexture(this, pixels, width, height, format);
}

uint32_t RvDevice::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
		if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
			return i;
		}
	}

	throw std::runtime_error("Failed to find suitable memory type!");
}

VkFormat RvDevice::findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
{
	for (VkFormat format : candidates) {
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);

		if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
			return format;
		}
		else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
			return format;
		}
	}

	throw std::runtime_error("Failed to find supported format!");
}

VkFormat RvDevice::findDepthFormat()
{
	return findSupportedFormat(
		{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
		VK_IMAGE_TILING_OPTIMAL,
		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

VkSampleCountFlagBits RvDevice::getMaxUsableSampleCount()
{
	VkSampleCountFlags counts = std::min(deviceProperties.limits.framebufferColorSampleCounts, deviceProperties.limits.framebufferDepthSampleCounts);
	if (counts & VK_SAMPLE_COUNT_64_BIT) { return VK_SAMPLE_COUNT_64_BIT; }
	if (counts & VK_SAMPLE_COUNT_32_BIT) { return VK_SAMPLE_COUNT_32_BIT; }
	if (counts & VK_SAMPLE_COUNT_16_BIT) { return VK_SAMPLE_COUNT_16_BIT; }
	if (counts & VK_SAMPLE_COUNT_8_BIT) { return VK_SAMPLE_COUNT_8_BIT; }
	if (counts & VK_SAMPLE_COUNT_4_BIT) { return VK_SAMPLE_COUNT_4_BIT; }
	if (counts & VK_SAMPLE_COUNT_2_BIT) { return VK_SAMPLE_COUNT_2_BIT; }

	return VK_SAMPLE_COUNT_1_BIT;
}

VkCommandBuffer RvDevice::beginSingleTimeCommands()
{
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = commandPool;
	allocInfo.commandBufferCount = 1;

	//TODO: We might want to have a separate command pool for this kind of short lived command buffer.
	//Reference: https://vulkan-tutorial.com/Vertex_buffers/Staging_buffer#page_Using_a_staging_buffer
	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(handle, &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT; //Telling the driver we're only using the buffer once

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	return commandBuffer;
}

void RvDevice::endSingleTimeCommands(VkCommandBuffer commandBuffer)
{
	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	//The graphics queue implictly has a transfer queue
	vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	//TODO: Could use a fence instead of waiting for the queue.
	//That would allow for scheduling multiple transfers simultaneously and waiting for all to complete.
	vkQueueWaitIdle(graphicsQueue);

	vkFreeCommandBuffers(handle, commandPool, 1, &commandBuffer);
}

VkFormatProperties RvDevice::getFormatProperties(VkFormat format)
{
	VkFormatProperties formatProperties;
	vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &formatProperties);
	return formatProperties;
}
