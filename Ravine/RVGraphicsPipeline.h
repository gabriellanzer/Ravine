#ifndef RV_GRAPHICS_PIPELINE_H
#define RV_GRAPHICS_PIPELINE_H

//Vulkan Includes
#include "volk.h"

//Ravine Systems
#include "RvDevice.h"
#include "RvRenderPass.h"

struct RvGraphicsPipeline
{
	//Internal States
	VkPipeline handle;
	VkPipelineCache pipelineCache;
	VkShaderStageFlagBits allAttachedShaderStageBits;
	vector<VkShaderStageFlagBits> attachedShaderStageBits;
	vector<const VkShaderModule*> attachedShaderModules;
	vector<const char*> shaderEntryPoints;

    const void*                                 pNext;
    VkPipelineCreateFlags                       createFlags;
    VkPipelineVertexInputStateCreateInfo		vertexInputState;
    VkPipelineInputAssemblyStateCreateInfo		inputAssemblyState;
    VkPipelineTessellationStateCreateInfo		tessellationState;
    VkPipelineViewportStateCreateInfo			viewportState;
    VkPipelineRasterizationStateCreateInfo		rasterizationState;
    VkPipelineMultisampleStateCreateInfo		multisampleState;
    VkPipelineDepthStencilStateCreateInfo		depthStencilState;
    VkPipelineColorBlendStateCreateInfo			colorBlendState;
    VkPipelineDynamicStateCreateInfo			dynamicState;
    VkPipelineLayout                            layout;

	const VkPipeline* pipelineInheritance;

	//External References
	RvDevice*									device;
	RvRenderPass								renderPass;
    uint32_t                                    subpass;
    VkPipeline                                  basePipelineHandle;
    int32_t                                     basePipelineIndex;

	RvGraphicsPipeline(VkPipeline basePipelineHandle = VK_NULL_HANDLE, int32_t basePipelineIndex = -1);
	~RvGraphicsPipeline();

	void create();
	void recreate();
	void attachShader(const VkShaderStageFlagBits stageFlagBits, const VkShaderModule* shaderModule, const char* entryPointName = "main");
	void forceAttachShader(const VkShaderStageFlagBits stageFlagBits, const VkShaderModule* shaderModule, const char* entryPointName = "main");
	void detachShader(const VkShaderModule* shaderModule);
	static RvGraphicsPipeline* defaultGraphicsPipeline(RvDevice& device, const RvRenderPass& renderPass);

	operator VkPipeline() const;
};

#endif