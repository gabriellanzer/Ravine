#ifndef RV_GRAPHICS_PIPELINE_H
#define RV_GRAPHICS_PIPELINE_H

//Vulkan Includes
#include "volk.h"

//Ravine Systems
#include "RvDevice.h"
#include "RvRenderPass.h"

struct RvGraphicsPipeline
{
	RvGraphicsPipeline();
	~RvGraphicsPipeline();

	void create();
	void recreate();
	void attachShader(const VkShaderStageFlagBits stageFlagBits, const VkShaderModule* shaderModule, const char* entryPointName = "main");
	void forceAttachShader(const VkShaderStageFlagBits stageFlagBits, const VkShaderModule* shaderModule, const char* entryPointName = "main");
	void deattachShader(const VkShaderModule* shaderModule);

	//Internal State
	VkPipeline handle;
	VkPipelineLayout layout;
	VkPipelineCache pipelineCache;
	VkShaderStageFlagBits allAttachedShaderStageBits;
	vector<VkShaderStageFlagBits> attachedShaderStageBits;
	vector<const VkShaderModule*> attachedShaderModules;
	vector<const char*> shaderEntryPoints;
	VkDescriptorSetLayout descriptorSetLayout;

	//External References
	RvDevice* device;
	RvRenderPass* renderPass;

	operator VkPipeline() const;
};

#endif