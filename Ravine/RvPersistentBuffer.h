#ifndef RV_PERSISTENT_BUFFER_H
#define RV_PERSISTENT_BUFFER_H

//Vulkan Include
#include "volk.h"

struct RvPersistentBuffer
{
	//Buffer size in bytes (so it's the whole buffer)
	RvPersistentBuffer();
	RvPersistentBuffer(VkDeviceSize bufferSize, size_t sizeOfDataType);
	~RvPersistentBuffer();

	VkBuffer handle = VK_NULL_HANDLE;
	VkDeviceMemory memory = VK_NULL_HANDLE;
	VkDeviceSize bufferSize = 0;
	size_t sizeOfDataType = 0;
	size_t instancesCount = 0;

	//TODO: Implement someday hehe
	// bool GetData(void* data, size_t offset, size_t);
};

#endif