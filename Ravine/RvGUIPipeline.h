#ifndef RV_GUI_PIPELINE_H
#define RV_GUI_PIPELINE_H

//Vulkan Includes
#include <vulkan\vulkan.h>

//Ravine Systems
#include "RvDevice.h"

struct RvGUIPipeline
{
	RvGUIPipeline(RvDevice& device, VkExtent2D extent, VkSampleCountFlagBits sampleCount, VkDescriptorSetLayout descriptorSetLayout, VkPushConstantRange* pushConstantRange, VkRenderPass renderPass);
	~RvGUIPipeline();

	RvDevice* device;
	VkPipeline handle;

	VkShaderModule vertModule;
	VkShaderModule fragModule;

	VkPipelineLayout layout;
	VkPipelineCache pipelineCache;

	operator VkPipeline() {
		return handle;
	}
};

#endif