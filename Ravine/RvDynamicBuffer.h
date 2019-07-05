#ifndef RV_DYNAMIC_BUFFER_H
#define RV_DYNAMIC_BUFFER_H

//Vulkan Include
#include <vulkan/vulkan.h>

struct RvDynamicBuffer
{
	RvDynamicBuffer();
	~RvDynamicBuffer();

	VkBuffer handle = VK_NULL_HANDLE;
	VkDeviceMemory memory = VK_NULL_HANDLE;
};

#endif