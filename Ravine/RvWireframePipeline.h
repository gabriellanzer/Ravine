#ifndef RV_WIREFRAME_PIPELINE_H
#define RV_WIREFRAME_PIPELINE_H

//Vulkan Includes
#include <vulkan\vulkan.h>

//Ravine Systems
#include "RvDevice.h"

struct RvWireframePipeline
{
	RvWireframePipeline(RvDevice& device, VkExtent2D extent, VkSampleCountFlagBits sampleCount, VkDescriptorSetLayout descriptorSetLayout, VkRenderPass renderPass, const vector<char>& vertShaderCode, const vector<char>& fragShaderCode);
	~RvWireframePipeline();

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