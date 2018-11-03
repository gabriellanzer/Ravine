#include "RvPersistentBuffer.h"


RvPersistentBuffer::RvPersistentBuffer()
{
}

RvPersistentBuffer::RvPersistentBuffer(VkDeviceSize bufferSize, size_t sizeOfDataType) :
	instancesCount(bufferSize / sizeOfDataType),
	bufferSize(bufferSize), sizeOfDataType(sizeOfDataType)
{


}


RvPersistentBuffer::~RvPersistentBuffer()
{
}