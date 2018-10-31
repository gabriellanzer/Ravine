#ifndef RV_FRAMEBUFFER_ATTACHMENT_H
#define RV_FRAMEBUFFER_ATTACHMENT_H

//Vulkan Include
#include <vulkan/vulkan.h>

struct RvFramebufferAttachment {
	VkImage image;
	VkDeviceMemory memory;
	VkImageView imageView;
	VkFormat format;
};

struct RvFramebufferAttachmentCreateInfo {
	uint32_t mipLevels;
	uint32_t layerCount;
	VkFormat format;

	VkImageTiling tilling;
	VkImageUsageFlags usage;
	VkMemoryPropertyFlagBits memoryProperties;
	VkImageAspectFlagBits aspectFlag;

	VkImageLayout initialLayout;
	VkImageLayout finalLayout;
};

#endif