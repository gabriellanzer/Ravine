#ifndef RV_RENDERPASS_H
#define RV_RENDERPASS_H

//Vulkan Includes
#include <vulkan/vulkan.h>

//EASTL Includes
#include <EASTL/vector.h>

using eastl::vector;

//Forward declaration for Subpass usage
struct RvSubpass;

struct RvRenderPass
{
private:

	//Renderpass
	VkRenderPass handle = VK_NULL_HANDLE;

	//Subpasses define implicit memory barriers and attachments
	vector<RvSubpass> subpasses;

public:

	RvRenderPass();
	~RvRenderPass();

	//Attaches a Subpass to event to this 
	void AttachSubpass(RvSubpass subpass);

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