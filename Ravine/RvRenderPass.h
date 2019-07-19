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
#include "RvSwapChain.h"
#include "RvTypeDefs.h"

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
	vector<RvAttachmentDescription> attachmentDescriptions;
	vector<RvFramebufferAttachment> framebufferAttachments;
	vector<RvFramebufferAttachment> sharedFramebufferAttachments;
	vector<RvFramebufferAttachmentCreateInfo> framebufferAttachmentsCreateInfos;
	vector<RvFramebufferAttachmentCreateInfo> sharedFramebufferAttachmentsCreateInfos;

	//RvDevice reference
	const RvDevice* device = nullptr;

	/**
	 * \brief Friend class to enable access of private attributes.
	 */
	friend class Ravine;
	friend class RvGui; //TODO: Change the way RvGui works to avoid this type of access

public:

	RvRenderPass() = default;
	~RvRenderPass() = default;

	/**
	 * \brief Actually RenderPass construction on given device for the number of desired frames.
	 * \param device The device to create the VkFrameBuffers and VkRenderPass instances.
	 * \param framesCount Amount of frames to be created based on RvFrameBufferAttachmentCreateInfo.
	 * \param sizeAndLayers Size of the framebuffer (width and height) and amount of layers (depth).
	 * \param swapchainImages Optional images array that will be appended to the VkFrameBufferAttachments
	 * with their respective attaching per frame (array size must match framesCount).
	 */
	void construct(const RvDevice& device, const uint32_t framesCount, const VkExtent3D& sizeAndLayers, const VkImageView* swapchainImages = VK_NULL_HANDLE);

	void resizeAttachments(const uint32_t framesCount, const VkExtent3D& sizeAndLayers, const VkImageView* swapchainImages = VK_NULL_HANDLE);

	/**
	 * \brief 
	 */
	void clear();
	
	/**
	 * \brief Attaches a Subpass into this RenderPass.
	 * \param subpass 
	 */
	void addSubpass(const RvSubpass subpass);

	/**
	 * \brief Add a framebuffer attachment create info that will be created for each of the frame buffers.
	 * \param createInfo Information required to create the framebuffer attachment.
	 */
	void addFramebufferAttachment(RvFramebufferAttachmentCreateInfo createInfo);

	/**
	 * \brief Add a framebuffer descriptor that defines what
	 * \param description The Description of a given attachment (order must match subpass usage description)
	 */
	void addAttachmentDescriptor(RvAttachmentDescription description);

	/**
	 * \brief 
	 * \param createInfo 
	 */
	void addSharedFramebufferAttachment(RvFramebufferAttachmentCreateInfo createInfo);

	static RvRenderPass* defaultRenderPass(RvDevice& device, const RvSwapChain& swapChain);
};

struct RvSubpass
{
	VkSubpassDescription description;
	VkSubpassDependency dependency;
	VkAttachmentReference* attachmentReferences = nullptr;
	uint32_t* preserveAttachments = nullptr;

	RvSubpass(const RvSubpass& subpass);
	RvSubpass(VkSubpassDescription descriptr, VkSubpassDependency dependency);
	~RvSubpass();
	static RvSubpass defaultSubpass();
};

#endif