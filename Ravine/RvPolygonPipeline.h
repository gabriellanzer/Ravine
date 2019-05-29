#ifndef RV_POLYGON_PIPELINE_H
#define RV_POLYGON_PIPELINE_H

//Vulkan Includes
#include <vulkan\vulkan.h>

//Ravine Systems
#include "RvDevice.h"

struct RvPolygonPipeline
{
	RvPolygonPipeline(RvDevice& device, VkExtent2D extent, VkSampleCountFlagBits sampleCount, VkDescriptorSetLayout* descriptorSetLayout, size_t descriptorSetLayoutCount, VkRenderPass renderPass, const vector<char>& vertShaderCode, const vector<char>& fragShaderCode);
	~RvPolygonPipeline();

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