#ifndef RV_TEXTURE_H
#define RV_TEXTURE_H

//Vulkan Includes
#include <vulkan/vulkan.h>

struct RvTexture
{
	RvTexture();
	~RvTexture();

	//The texture size extent
	VkExtent2D extent;

	//Buffer size in bytes
	size_t dataSize;

	//Amount of mipLevels for this texture
	uint32_t mipLevels;

	//The GPU device that holds this image
	VkDevice device;

	//The VkImage handle
	VkImage handle;
	
	//The VkImageView handle
	VkImageView view;

	//The memory handle on a GPU device
	VkDeviceMemory memory;

	void Free();

	operator VkImage()
	{
		return handle;
	}

};

#endif