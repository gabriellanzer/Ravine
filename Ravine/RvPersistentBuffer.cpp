#include "RvPersistentBuffer.h"



RvPersistentBuffer::RvPersistentBuffer(void* data, size_t bufferSize, size_t sizeOfDataType) : instancesCount(bufferSize / sizeOfDataType)
{
	//VkBuffer stagingBuffer;
	//VkDeviceMemory stagingBufferMemory;
	////Staging buffer is being used as the source in an a memory transfer operation
	//createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	////Fetching verteices data
	//void* data;
	//vkMapMemory(*device, stagingBufferMemory, 0, bufferSize, 0, &data);
	//memcpy(data, (void*)meshes[0].vertices, (size_t)bufferSize);
	//vkUnmapMemory(*device, stagingBufferMemory);

	////Verter buffer is being used as the destination for such memory transfer operation
	//createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory);

	////Transfering vertices to index buffer
	//copyBuffer(stagingBuffer, vertexBuffer, bufferSize);

	////Clearing staging buffer
	//vkDestroyBuffer(*device, stagingBuffer, nullptr);
	//vkFreeMemory(*device, stagingBufferMemory, nullptr);

}


RvPersistentBuffer::~RvPersistentBuffer()
{
}