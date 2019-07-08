#ifndef RV_GRAPHICS_PIPELINE_H
#define RV_GRAPHICS_PIPELINE_H

//Vulkan Includes
#include "volk.h"

//Ravine Systems
#include "RvDevice.h"

struct RvGraphicsPipeline
{
	RvGraphicsPipeline(VkDescriptorSetLayout descriptorSetLayout, const vector<char>& vertShaderCode, const vector<char>& fragShaderCode);
	~RvGraphicsPipeline();

	RvDevice* device;
	VkPipeline handle;

	VkShaderModule vertModule;
	VkShaderModule fragModule;

	VkPipelineLayout layout;
	VkPipelineCache pipelineCache;
	VkRenderPass renderPass;

	operator VkPipeline() {
		return handle;
	}
};

#endif