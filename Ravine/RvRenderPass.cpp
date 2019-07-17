#include "RvRenderPass.h"
#include "RvTools.h"
#include <stdexcept>

RvRenderPass::RvRenderPass()
= default;

RvRenderPass::~RvRenderPass()
= default;

void RvRenderPass::construct(const RvDevice& device, const uint32_t framesCount, const VkExtent3D& sizeAndLayers, const VkImageView* swapchainImages)
{
	this->device = &device;

	//Resize array to allocate required VkFramebuffer objects
	framebuffers.resize(framesCount);

	//Create all shared framebuffer attachments
	sharedFramebufferAttachments.reserve(sharedFramebufferAttachmentsCreateInfos.size());
	for (auto createInfo : sharedFramebufferAttachmentsCreateInfos)
	{
		sharedFramebufferAttachments.push_back(rvTools::createFramebufferAttachment(device, createInfo));
	}

	//For each frame, create a framebuffer with it's attachments
	for (uint32_t frameIt = 0; frameIt < framesCount; frameIt++)
	{
		vector<VkImageView> attachments;

		//Link all shared attachments
		for (RvFramebufferAttachment attachment : sharedFramebufferAttachments)
		{
			attachments.push_back(attachment.imageView);
		}

		//Create and link individual attachments
		for (auto createInfo : framebufferAttachmentsCreateInfos)
		{
			RvFramebufferAttachment attachment = rvTools::createFramebufferAttachment(device, createInfo);
			framebufferAttachments.push_back(attachment);
			attachments.push_back(attachment.imageView);
		}

		//Link proper swapchain image attachment
		if(swapchainImages != VK_NULL_HANDLE)
		{
			attachments.push_back(swapchainImages[frameIt]);
		}

		VkFramebufferCreateInfo framebufferCreateInfo = {};
		framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferCreateInfo.renderPass = handle;
		framebufferCreateInfo.attachmentCount = attachments.size();
		framebufferCreateInfo.pAttachments = attachments.data();
		framebufferCreateInfo.width = sizeAndLayers.width;
		framebufferCreateInfo.height = sizeAndLayers.height;
		framebufferCreateInfo.layers = sizeAndLayers.depth;

		if (vkCreateFramebuffer(device.handle, &framebufferCreateInfo, nullptr, &framebuffers[frameIt]) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create framebuffer!");
		}
	}
}

void RvRenderPass::clear()
{
	//Destroy shared attachments
	for (auto& sharedFramebufferAttachment : sharedFramebufferAttachments)
	{
		vkDestroyImageView(device->handle, sharedFramebufferAttachment.imageView, nullptr);
		vkDestroyImage(device->handle, sharedFramebufferAttachment.image, nullptr);
		vkFreeMemory(device->handle, sharedFramebufferAttachment.memory, nullptr);
	}

	//Destroy attachments
	for (auto& framebufferAttachment : framebufferAttachments)
	{
		vkDestroyImageView(device->handle, framebufferAttachment.imageView, nullptr);
		vkDestroyImage(device->handle, framebufferAttachment.image, nullptr);
		vkFreeMemory(device->handle, framebufferAttachment.memory, nullptr);
	}

	//Destroy FrameBuffers
	for (VkFramebuffer framebuffer : framebuffers) {
		vkDestroyFramebuffer(device->handle, framebuffer, nullptr);
	}

	vkDestroyRenderPass(device->handle, handle, nullptr);
}

void RvRenderPass::linkSubpass(const RvSubpass& subpass)
{
	subpasses.push_back(subpass);
}

void RvRenderPass::linkFramebufferAttachment(RvFramebufferAttachment attachment)
{
	framebufferAttachments.push_back(attachment);
}

void RvRenderPass::linkSharedFramebufferAttachment(RvFramebufferAttachment createInfo)
{
	sharedFramebufferAttachments.push_back(attachment);
}

RvSubpass::RvSubpass() : description(), dependency()
{
	//Default constructor aligns with default renderpass construction
	description.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	description.colorAttachmentCount = 1;

	//Color buffer attachment
	VkAttachmentReference colorAttachmentRef = {};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	description.pColorAttachments = &colorAttachmentRef;

	//Depth-stencil buffer attachment
	VkAttachmentReference depthAttachmentRef = {};
	depthAttachmentRef.attachment = 1;
	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	description.pDepthStencilAttachment = &depthAttachmentRef;

	//Color resolve attachment (fast MSAA)
	VkAttachmentReference colorAttachmentResolveRef = {};
	colorAttachmentResolveRef.attachment = 2;
	colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	description.pResolveAttachments = &colorAttachmentResolveRef;

	//Default subpass dependency (based on single subpass renderpass)
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;

	//What we will wait for
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;

	//What we will do with
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

}

RvSubpass::~RvSubpass()
= default;
