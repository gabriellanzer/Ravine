#ifndef RV_TEXTURE_H
#define RV_TEXTURE_H

//Vulkan Includes
#include <vulkan/vulkan.h>

#pragma region USEFULL DEFINES
#define VK_CHECK_RESULT(f)																				\
{																										\
	VkResult res = (f);																					\
	if (res != VK_SUCCESS)																				\
	{																									\
		std::cout << "Fatal : VkResult is \"" << vks::tools::errorString(res) << "\" in " << __FILE__ << " at line " << __LINE__ << std::endl; \
		assert(res == VK_SUCCESS);																		\
	}																									\
}
#pragma endregion

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