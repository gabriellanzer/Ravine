#ifndef RV_RENDERPASS_H
#define RV_RENDERPASS_H

//Vulkan Includes
#include <vulkan/vulkan.h>

//EASTL Includes
#include <eastl/vector.h>

using eastl::vector;

//Ravine Includes
#include "RvDevice.h"
#include "RvSwapChain.h"
#include "RvFramebufferAttachment.h"

//Forward declaration for Subpass usage
struct RvSubpass;

struct RvRenderPass
{
private:

	//Renderpass
	VkRenderPass handle = VK_NULL_HANDLE;

	//Subpasses define implicit memory barriers and attachments
	vector<RvSubpass> subpasses;

	//Framebuffers and their attachments
	vector<VkFramebuffer> framebuffers;
	vector<RvFramebufferAttachment> framebufferAttachments;

	//Proper creation of framebuffers for each of the SwapChain images
	void CreateFramebuffers(const vector<VkImage>& swapChainImages);

public:

	RvRenderPass();
	~RvRenderPass();

	//Actually RenderPass construction on given device, based on SwapChain extent
	//this call also constructs the framebuffers attached to this RenderPass in
	//such a way that they are replicated accordingly to inflight frames defined
	//by the swapchain
	void Construct(const RvDevice& device, const RvSwapChain& swapchain);

	//Attaches a Subpass into this RenderPass
	void AttachSubpass(RvSubpass subpass);

	//Creates a framebuffer attachment that will be added into each framebuffer
	//once the RenderPass actually gets created
	void AddFramebufferAttachment(RvFramebufferAttachmentCreateInfo createInfo);

};

struct RvSubpass
{
private:

	VkSubpassDescription description;
	VkSubpassDependency dependency;

public:

	RvSubpass();
	~RvSubpass();

};

#endif