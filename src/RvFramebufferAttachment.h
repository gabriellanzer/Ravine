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

	VkExtent3D extent;
};

const static RvFramebufferAttachmentCreateInfo rvDefaultDepthAttachment
{
	1,
	1,
	VK_FORMAT_MAX_ENUM, //Must be defined manually
	VK_IMAGE_TILING_OPTIMAL,
	VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
	VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
	VK_IMAGE_ASPECT_DEPTH_BIT,
	static_cast<VkImageCreateFlagBits>(0), //No ImageCreateFlag bits defined
	VK_IMAGE_LAYOUT_UNDEFINED,
	VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
	{0, 0, 1} //Must be defined manually
};

const static RvFramebufferAttachmentCreateInfo rvDefaultResolveAttachment
{
	1,
	1,
	VK_FORMAT_MAX_ENUM, //Must be defined manually
	VK_IMAGE_TILING_OPTIMAL,
	VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
	VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
	VK_IMAGE_ASPECT_COLOR_BIT,
	static_cast<VkImageCreateFlagBits>(0), //No ImageCreateFlag bits defined
	VK_IMAGE_LAYOUT_UNDEFINED,
	VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
	{0, 0, 1} //Must be defined manually
};

const static RvFramebufferAttachmentCreateInfo rvDefaultColorAttachment
{
	1,
	1,
	VK_FORMAT_MAX_ENUM, //Must be defined manually
	VK_IMAGE_TILING_OPTIMAL,
	VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
	VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
	VK_IMAGE_ASPECT_COLOR_BIT,
	static_cast<VkImageCreateFlagBits>(0), //No ImageCreateFlag bits defined
	VK_IMAGE_LAYOUT_UNDEFINED,
	VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
	{0, 0, 1} //Must be defined manually
};

#endif