#ifndef RV_GUI_PIPELINE_H
#define RV_GUI_PIPELINE_H

//Vulkan Includes
#include "volk.h"

//Ravine Systems
#include "RvDevice.h"

struct RvGuiPipeline
{
	RvGuiPipeline(RvDevice& device, VkExtent2D extent, VkSampleCountFlagBits sampleCount, VkDescriptorSetLayout descriptorSetLayout, VkPushConstantRange* pushConstantRange, VkRenderPass renderPass);
	~RvGuiPipeline();

	RvDevice* device;
	VkPipeline handle;

	VkShaderModule vertModule;
	VkShaderModule fragModule;

	VkPipelineLayout layout;
	VkPipelineCache pipelineCache;

	explicit operator VkPipeline() const;
};

#endif