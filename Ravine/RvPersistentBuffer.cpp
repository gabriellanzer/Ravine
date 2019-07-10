#include "RvPersistentBuffer.h"


RvPersistentBuffer::RvPersistentBuffer()
= default;

RvPersistentBuffer::RvPersistentBuffer(VkDeviceSize bufferSize, size_t sizeOfDataType) :
	bufferSize(bufferSize), sizeOfDataType(sizeOfDataType),
	instancesCount(bufferSize / sizeOfDataType)
{


}


RvPersistentBuffer::~RvPersistentBuffer()
= default;
