#include "RvDynamicBuffer.h"

RvDynamicBuffer::RvDynamicBuffer()
= default;

RvDynamicBuffer::RvDynamicBuffer(const VkDeviceSize& bufferSize) : bufferSize(bufferSize)
{

}

RvDynamicBuffer::~RvDynamicBuffer()
= default;
