#ifndef RV_DYNAMIC_BUFFER_H
#define RV_DYNAMIC_BUFFER_H

//Vulkan Include
#include <vulkan/vulkan.h>

struct RvDynamicBuffer
{
	RvDynamicBuffer();
	~RvDynamicBuffer();

	VkBuffer buffer;
	VkDeviceMemory memory;
};

#endif