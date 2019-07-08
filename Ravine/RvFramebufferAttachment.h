#ifndef RV_FRAMEBUFFER_ATTACHMENT_H
#define RV_FRAMEBUFFER_ATTACHMENT_H

//Vulkan Include
#include "volk.h"

struct RvFramebufferAttachment {
	VkImage image;
	VkDeviceMemory memory;
	VkImageView imageView;
};

struct RvFramebufferAttachmentCreateInfo {
	uint32_t mipLevels;
	uint32_t layerCount;
	VkFormat format;

	VkImageTiling tilling;
	VkImageUsageFlags usage;
	VkMemoryPropertyFlagBits memoryProperties;
	VkImageAspectFlagBits aspectFlag;
	VkImageCreateFlagBits createFlag;

	VkImageLayout initialLayout;
	VkImageLayout finalLayout;
};

#endif