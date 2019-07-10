#include "RvTexture.h"



RvTexture::RvTexture()
= default;


RvTexture::~RvTexture()
= default;

void RvTexture::Free()
{
	//Destroy handles in proper dependency order
	vkDestroyImageView(device, view, nullptr);
	vkDestroyImage(device, handle, nullptr);
	vkFreeMemory(device, memory, nullptr);
}
