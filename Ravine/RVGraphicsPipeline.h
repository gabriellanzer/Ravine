#ifndef RV_GRAPHICS_PIPELINE_H
#define RV_GRAPHICS_PIPELINE_H

//Vulkan Includes
#include "volk.h"

//Ravine Systems
#include "RvDevice.h"
#include "RvRenderPass.h"

struct RvGraphicsPipeline
{
	//External References
	const RvDevice*								device							= nullptr;
	RvRenderPass								renderPass						= {};
    uint32_t                                    subpass							= 0;
    VkPipeline                                  basePipeline					= VK_NULL_HANDLE;
	vector<VkDescriptorSetLayout>				descriptorSetLayouts			= {};
	vector<VkPushConstantRange>					pushConstantRanges				= {};

	//Internal States
	VkPipeline									handle							= VK_NULL_HANDLE;
	VkPipelineCache								pipelineCache					= VK_NULL_HANDLE;
	VkPipelineLayout                            layout							= VK_NULL_HANDLE;
	VkShaderStageFlagBits						allAttachedShaderStageBits		= static_cast<VkShaderStageFlagBits>(0);
	vector<VkShaderStageFlagBits>				attachedShaderStageBits			= {};
	vector<const VkShaderModule*>				attachedShaderModules			= {};
	vector<const char*>							shaderEntryPoints				= {};
    VkPipelineCreateFlags                       createFlags						= static_cast<VkPipelineCreateFlags>(0);
    vector<VkVertexInputBindingDescription>		vertexBindingDescriptions		= {};
    vector<VkVertexInputAttributeDescription>	vertexAttributeDescriptions		= {};
    VkPipelineVertexInputStateCreateInfo		vertexInputState				= {};
    VkPipelineInputAssemblyStateCreateInfo		inputAssemblyState				= {};
    VkPipelineTessellationStateCreateInfo		tessellationState				= {};
    vector<VkViewport>							viewports						= {};
	vector<VkRect2D>							scissors						= {};
    VkPipelineViewportStateCreateInfo			viewportState					= {};
    VkPipelineRasterizationStateCreateInfo		rasterizationState				= {};
    VkSampleMask								sampleMask						= static_cast<VkSampleMask>(0);
    VkPipelineMultisampleStateCreateInfo		multisampleState				= {};
    VkPipelineDepthStencilStateCreateInfo		depthStencilState				= {};
    vector<VkPipelineColorBlendAttachmentState>	colorBlendAttachments			= {};
	VkPipelineColorBlendStateCreateInfo			colorBlendState					= {};
	vector<VkDynamicState>						dynamicStates					= {};
    VkPipelineDynamicStateCreateInfo			dynamicState					= {};

	explicit RvGraphicsPipeline(const RvDevice* device, VkPipeline basePipeline = VK_NULL_HANDLE);
	~RvGraphicsPipeline();

	void create();
	void recreate();
	void attachShader(const VkShaderStageFlagBits stageFlagBits, const VkShaderModule* shaderModule, const char* entryPointName = "main");
	void forceAttachShader(const VkShaderStageFlagBits stageFlagBits, const VkShaderModule* shaderModule, const char* entryPointName = "main");
	void detachShader(const VkShaderModule* shaderModule);
	static RvGraphicsPipeline* defaultGraphicsPipeline(RvDevice& device, const RvRenderPass& renderPass);
	static VkPipelineCache defaultPipelineCache(RvDevice& device);

	operator VkPipeline() const;
};

#endif