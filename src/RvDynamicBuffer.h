#ifndef RV_DYNAMIC_BUFFER_H
#define RV_DYNAMIC_BUFFER_H

//Vulkan Include
#include "volk.h"

struct RvDynamicBuffer
{
	RvDynamicBuffer();
	RvDynamicBuffer(const VkDeviceSize& bufferSize);
	~RvDynamicBuffer();

	VkBuffer handle = VK_NULL_HANDLE;
	VkDeviceMemory memory = VK_NULL_HANDLE;
	VkDeviceSize bufferSize = 0;
	VkDeviceSize allocatedSize = 0;
};

#endif