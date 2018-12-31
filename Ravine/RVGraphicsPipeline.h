#ifndef RV_GRAPHICS_PIPELINE_H
#define RV_GRAPHICS_PIPELINE_H

//Vulkan Includes
#include <vulkan\vulkan.h>

//Ravine Systems
#include "RvDevice.h"

struct RvGraphicsPipeline
{
	RvGraphicsPipeline(RvDevice& device, VkExtent2D extent, VkSampleCountFlagBits sampleCount, VkDescriptorSetLayout descriptorSetLayout, VkPushConstantRange pushConstantRange, VkRenderPass renderPass);
	~RvGraphicsPipeline();

	RvDevice* device;
	VkPipeline handle;

	VkShaderModule vertModule;
	VkShaderModule fragModule;

	VkPipelineLayout pipelineLayout;
	VkPipelineCache pipelineCache;
	VkRenderPass renderPass;

	void Init(VkExtent2D extent, VkSampleCountFlagBits sampleCount, VkDescriptorSetLayout descriptorSetLayout, VkPushConstantRange pushConstantRange, VkRenderPass renderPass);

	operator VkPipeline() {
		return handle;
	}
};

#endif