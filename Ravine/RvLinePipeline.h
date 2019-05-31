#ifndef RV_LINE_PIPELINE_H
#define RV_LINE_PIPELINE_H

//Vulkan Includes
#include <vulkan\vulkan.h>

//Ravine Systems
#include "RvDevice.h"

struct RvLinePipeline
{
	RvLinePipeline(RvDevice& device, VkExtent2D extent, VkSampleCountFlagBits sampleCount, VkDescriptorSetLayout* descriptorSetLayout, size_t descriptorSetLayoutCount, VkRenderPass renderPass, const vector<char>& vertShaderCode, const vector<char>& fragShaderCode);
	~RvLinePipeline();

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