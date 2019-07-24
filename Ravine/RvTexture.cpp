#include "RvTexture.h"

void RvTexture::free()
{
	//Destroy handles in proper dependency order
	vkDestroyImageView(device, view, nullptr);
	vkDestroyImage(device, handle, nullptr);
	vkFreeMemory(device, memory, nullptr);
}
