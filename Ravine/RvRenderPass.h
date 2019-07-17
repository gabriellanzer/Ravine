#ifndef RV_RENDERPASS_H
#define RV_RENDERPASS_H

//Vulkan Includes
#include "volk.h"

//EASTL Includes
#include <eastl/vector.h>

using eastl::vector;

//Ravine Includes
#include "RvDevice.h"
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
	vector<RvFramebufferAttachment> sharedFramebufferAttachments;

	//RvDevice reference
	const RvDevice* device = nullptr;

public:

	RvRenderPass();
	~RvRenderPass();

	/**
	 * \brief Actually RenderPass construction on given device for the number of desired frames.
	 * \param device The device to create the VkFrameBuffers and VkRenderPass instances.
	 * \param framesCount Amount of frames to be created based on RvFrameBufferAttachmentCreateInfo.
	 * \param sizeAndLayers Size of the framebuffer (width and height) and amount of layers (depth).
	 * \param swapchainImages Optional images array that will be appended to the VkFrameBufferAttachments
	 * with their respective attaching per frame (array size must match framesCount).
	 */
	void construct(const RvDevice& device, const uint32_t framesCount, const VkExtent3D& sizeAndLayers, const VkImageView* swapchainImages = VK_NULL_HANDLE);

	/**
	 * \brief 
	 */
	void clear();
	
	/**
	 * \brief Attaches a Subpass into this RenderPass.
	 * \param subpass 
	 */
	void linkSubpass(const RvSubpass& subpass);

	/**
	 * \brief Creates a framebuffer attachment that will be added into each framebuffer
	 * once the RenderPass actually gets created
	 * \param createInfo 
	 */
	void linkFramebufferAttachment(RvFramebufferAttachment createInfo);

	/**
	 * \brief 
	 * \param createInfo 
	 */
	void linkSharedFramebufferAttachment(RvFramebufferAttachment createInfo);
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