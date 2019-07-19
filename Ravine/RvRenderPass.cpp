#include "RvRenderPass.h"
#include "RvTools.h"
#include <stdexcept>
#include <eastl/array.h>

void RvRenderPass::construct(const RvDevice& device, const uint32_t framesCount, const VkExtent3D& sizeAndLayers, const VkImageView* swapchainImages)
{
	this->device = &device;

	//Create RenderPass
	VkRenderPassCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	createInfo.attachmentCount = static_cast<uint32_t>(attachmentDescriptions.size());
	createInfo.pAttachments = attachmentDescriptions.data();
	const uint32_t subpassSize = static_cast<uint32_t>(subpasses.size());
	VkSubpassDescription* subpassDescriptions = new VkSubpassDescription[subpassSize];
	VkSubpassDependency* subpassDependencies = new VkSubpassDependency[subpassSize];
	for (uint32_t i = 0; i < subpassSize; i++)
	{
		subpassDescriptions[i] = subpasses[i].description;
		subpassDependencies[i] = subpasses[i].dependency;
	}
	createInfo.subpassCount = subpassSize;
	createInfo.pSubpasses = subpassDescriptions;
	createInfo.dependencyCount = subpassSize;
	createInfo.pDependencies = subpassDependencies;

	if (vkCreateRenderPass(device.handle, &createInfo, nullptr, &handle) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create render pass!");
	}

	//Resize array to allocate required VkFramebuffer objects
	framebuffers.resize(framesCount);

	//Create all shared framebuffer attachments
	sharedFramebufferAttachments.reserve(sharedFramebufferAttachmentsCreateInfos.size());
	for (auto attachmentCreateInfo : sharedFramebufferAttachmentsCreateInfos)
	{
		sharedFramebufferAttachments.push_back(rvTools::createFramebufferAttachment(device, attachmentCreateInfo));
	}

	//For each frame, create a framebuffer with it's attachments
	for (uint32_t frameIt = 0; frameIt < framesCount; frameIt++)
	{
		const uint32_t attachmentCount = (
			(swapchainImages != VK_NULL_HANDLE ? 1 : 0) + 
			sharedFramebufferAttachments.size() + 
			framebufferAttachmentsCreateInfos.size()
			);
		VkImageView* attachments = new VkImageView[attachmentCount];
		uint32_t it = 0;

		//Link all shared attachments
		for (uint32_t i = 0; i < sharedFramebufferAttachments.size(); ++i)
		{
			attachments[it+i] = sharedFramebufferAttachments[i].imageView;
		}
		it += sharedFramebufferAttachments.size();

		//Create and link individual attachments
		for (uint32_t i = 0; i < framebufferAttachmentsCreateInfos.size(); ++i)
		{
			RvFramebufferAttachment attachment = rvTools::createFramebufferAttachment(device, framebufferAttachmentsCreateInfos[i]);
			framebufferAttachments.push_back(attachment);
			attachments[it+i] = attachment.imageView;
		}

		//Link proper swapchain image attachment
		if (swapchainImages != VK_NULL_HANDLE)
		{
			attachments[it] = swapchainImages[frameIt];
			it++;
		}

		VkFramebufferCreateInfo framebufferCreateInfo = {};
		framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferCreateInfo.renderPass = handle;
		framebufferCreateInfo.attachmentCount = attachmentCount;
		framebufferCreateInfo.pAttachments = attachments;
		framebufferCreateInfo.width = sizeAndLayers.width;
		framebufferCreateInfo.height = sizeAndLayers.height;
		framebufferCreateInfo.layers = sizeAndLayers.depth;
		framebufferCreateInfo.pNext = nullptr;

		framebuffers[frameIt] = {};
		const VkResult result = vkCreateFramebuffer(device.handle, &framebufferCreateInfo, nullptr, &framebuffers[frameIt]);
		if (result != VK_SUCCESS) {
			throw std::runtime_error("Failed to create framebuffer!");
		}
	}
}

void RvRenderPass::clear()
{
	//Destroy shared attachments
	sharedFramebufferAttachmentsCreateInfos.clear();
	for (auto& sharedFramebufferAttachment : sharedFramebufferAttachments)
	{
		vkDestroyImageView(device->handle, sharedFramebufferAttachment.imageView, nullptr);
		vkDestroyImage(device->handle, sharedFramebufferAttachment.image, nullptr);
		vkFreeMemory(device->handle, sharedFramebufferAttachment.memory, nullptr);
	}
	sharedFramebufferAttachments.clear();

	//Destroy attachments
	framebufferAttachmentsCreateInfos.clear();
	for (auto& framebufferAttachment : framebufferAttachments)
	{
		vkDestroyImageView(device->handle, framebufferAttachment.imageView, nullptr);
		vkDestroyImage(device->handle, framebufferAttachment.image, nullptr);
		vkFreeMemory(device->handle, framebufferAttachment.memory, nullptr);
	}
	framebufferAttachments.clear();

	//Destroy FrameBuffers
	for (VkFramebuffer framebuffer : framebuffers) {
		vkDestroyFramebuffer(device->handle, framebuffer, nullptr);
	}
	framebuffers.clear();

	//Clear attachment description
	attachmentDescriptions.clear();
	subpasses.clear();

	//Actual RenderPass destruction
	vkDestroyRenderPass(device->handle, handle, nullptr);
}

void RvRenderPass::addSubpass(const RvSubpass subpass)
{
	subpasses.push_back(subpass);
}

void RvRenderPass::addFramebufferAttachment(const RvFramebufferAttachmentCreateInfo createInfo)
{
	sharedFramebufferAttachmentsCreateInfos.push_back(createInfo);
}

void RvRenderPass::addAttachmentDescriptor(const RvAttachmentDescription description)
{
	attachmentDescriptions.push_back(description);
}

void RvRenderPass::addSharedFramebufferAttachment(const RvFramebufferAttachmentCreateInfo createInfo)
{
	sharedFramebufferAttachmentsCreateInfos.push_back(createInfo);
}

RvRenderPass* RvRenderPass::defaultRenderPass(RvDevice& device, const RvSwapChain& swapChain)
{
	static RvRenderPass* renderPass = nullptr;

	if (renderPass != nullptr)
		return renderPass;

	renderPass = new RvRenderPass();

	//Setup Attachments
	const VkExtent3D size = { swapChain.extent.width, swapChain.extent.height, 1 };
	RvFramebufferAttachmentCreateInfo msaaCreateInfo = rvDefaultResolveAttachment;
	msaaCreateInfo.extent = size;
	msaaCreateInfo.format = swapChain.imageFormat;
	renderPass->addSharedFramebufferAttachment(msaaCreateInfo);
	RvFramebufferAttachmentCreateInfo depthCreateInfo = rvDefaultDepthAttachment;
	depthCreateInfo.extent = size;
	depthCreateInfo.format = device.findDepthFormat();
	renderPass->addSharedFramebufferAttachment(depthCreateInfo);

	//Color Attachment description
	//Reference: https://vulkan-tutorial.com/Drawing_a_triangle/Graphics_pipeline_basics/Render_passes#page_Attachment_description
	VkAttachmentDescription colorAttachment = {};
	colorAttachment.format = swapChain.imageFormat;	//Formats should match
	colorAttachment.samples = device.sampleCount;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	renderPass->addAttachmentDescriptor(colorAttachment);

	//Depth attachment description
	//Reference: https://vulkan-tutorial.com/Depth_buffering#page_Render_pass
	VkAttachmentDescription depthAttachment = {};
	depthAttachment.format = device.findDepthFormat();
	depthAttachment.samples = device.sampleCount;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	renderPass->addAttachmentDescriptor(depthAttachment);

	//Multi-sampled image resolving
	VkAttachmentDescription colorResolveAttachment = {};
	colorResolveAttachment.format = swapChain.imageFormat;
	colorResolveAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorResolveAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorResolveAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorResolveAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorResolveAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorResolveAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorResolveAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	renderPass->addAttachmentDescriptor(colorResolveAttachment);

	//Setup Default Subpass
	renderPass->addSubpass(RvSubpass::defaultSubpass());

	//Proper Creation
	renderPass->construct(device, swapChain.images.size(), size, swapChain.imageViews.data());

	return renderPass;
}

RvSubpass::RvSubpass(const RvSubpass& subpass) : RvSubpass(subpass.description, subpass.dependency)
{

}

RvSubpass::RvSubpass(VkSubpassDescription descriptr, VkSubpassDependency dependency) : description(descriptr), dependency(dependency)
{
	//Copy attachment references and rewire pointers
	attachmentReferences = new VkAttachmentReference[description.inputAttachmentCount +
		description.colorAttachmentCount * 3];

	//Use another pointer to iterate over memory blocks
	VkAttachmentReference* refPtr = attachmentReferences;

	//Input attachments
	if(description.inputAttachmentCount > 0)
	{
		memcpy(refPtr, description.pInputAttachments, description.inputAttachmentCount * sizeof(VkAttachmentReference));
		description.pInputAttachments = refPtr;
		refPtr += description.inputAttachmentCount;
	}

	//Color attachments
	memcpy(refPtr, description.pColorAttachments, description.colorAttachmentCount * sizeof(VkAttachmentReference));
	description.pColorAttachments = refPtr;
	refPtr += description.colorAttachmentCount;

	//Depth attachments
	memcpy(refPtr, description.pDepthStencilAttachment, description.colorAttachmentCount * sizeof(VkAttachmentReference));
	description.pDepthStencilAttachment = refPtr;
	refPtr += description.colorAttachmentCount;

	//Resolve attachments
	memcpy(refPtr, description.pResolveAttachments, description.colorAttachmentCount * sizeof(VkAttachmentReference));
	description.pResolveAttachments = refPtr;

	//Preserve attachments
	if(description.preserveAttachmentCount > 0)
	{
		preserveAttachments = new uint32_t[description.preserveAttachmentCount];
		memcpy(preserveAttachments, description.pPreserveAttachments, description.preserveAttachmentCount * 4);
		description.pPreserveAttachments = preserveAttachments;
	}
}

RvSubpass::~RvSubpass()
{
	delete[] attachmentReferences;
	delete[] preserveAttachments;
}

RvSubpass RvSubpass::defaultSubpass()
{
	//Default constructor aligns with default RenderPass construction
	VkSubpassDescription description = {};
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

	//Default subpass dependency (based on single subpass)
	VkSubpassDependency dependency = {};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;

	//What we will wait for
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;

	//What we will do with
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	return RvSubpass(description, dependency);
}
