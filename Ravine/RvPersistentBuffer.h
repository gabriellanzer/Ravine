#ifndef RV_PERSISTENT_BUFFER_H
#define RV_PERSISTENT_BUFFER_H

//Vulkan Include
#include <vulkan/vulkan.h>

struct RvPersistentBuffer
{
	//Buffer size in bytes (so it's the whole buffer)
	RvPersistentBuffer();
	RvPersistentBuffer(VkDeviceSize bufferSize, size_t sizeOfDataType);
	~RvPersistentBuffer();

	VkBuffer handle;
	VkDeviceMemory memory;
	VkDeviceSize bufferSize;
	size_t sizeOfDataType;
	size_t instancesCount;

	//TODO: Implement someday hehe
	// bool GetData(void* data, size_t offset, size_t);
};

#endif