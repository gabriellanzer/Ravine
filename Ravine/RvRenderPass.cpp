#include "RvRenderPass.h"

RvRenderPass::RvRenderPass()
= default;

RvRenderPass::~RvRenderPass()
= default;

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
