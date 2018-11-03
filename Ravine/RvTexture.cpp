#include "RvTexture.h"



RvTexture::RvTexture()
{
}


RvTexture::~RvTexture()
{

}

void RvTexture::Free()
{
	//Destroy handles in proper dependency order
	vkDestroyImageView(device, view, nullptr);
	vkDestroyImage(device, handle, nullptr);
	vkFreeMemory(device, memory, nullptr);
}
