#ifndef RV_GUI_PIPELINE_H
#define RV_GUI_PIPELINE_H

//Vulkan Includes
#include <vulkan\vulkan.h>

//Ravine Systems
#include "RvDevice.h"
#include "RvTools.h"
#include "RvDataTypes.h"

struct RvGUIPipeline
{
	RvGUIPipeline(RvDevice& device, VkExtent2D extent, VkSampleCountFlagBits sampleCount, VkDescriptorSetLayout descriptorSetLayout, VkRenderPass renderPass);
	~RvGUIPipeline();

	RvDevice* device;
	VkPipeline handle;

	VkShaderModule vertModule;
	VkShaderModule fragModule;

	VkPipelineLayout pipelineLayout;
	VkRenderPass renderPass;

	void Init(VkExtent2D extent, VkSampleCountFlagBits sampleCount, VkDescriptorSetLayout descriptorSetLayout, VkRenderPass renderPass);

	operator VkPipeline() {
		return handle;
	}
};

#endif