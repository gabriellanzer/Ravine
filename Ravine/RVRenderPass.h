#ifndef RV_RENDER_PASS_H
#define RV_RENDER_PASS_H

//Vulkan Include
#include <vulkan/vulkan.h>

//Ravine includes
#include "RVDevice.h"
#include "RVSwapChain.h"

struct RvRenderPass
{
	RvRenderPass(RvDevice& device, RvSwapChain& swapChain);
	~RvRenderPass();

	VkRenderPass handle;
	RvDevice* device;
	RvSwapChain* swapChain;

	void Init();

	operator VkRenderPass() {
		return handle;
	}
};

#endif