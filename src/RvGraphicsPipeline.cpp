#include "RvGraphicsPipeline.h"

#include <fmt/printf.h>
#include "RvTools.h"

RvGraphicsPipeline::RvGraphicsPipeline(const RvDevice* device, const VkPipeline basePipeline) : device(device), basePipeline(basePipeline)
{

}

RvGraphicsPipeline::~RvGraphicsPipeline()
{
	vkDestroyPipelineLayout(device->handle, layout, nullptr);
	vkDestroyPipeline(device->handle, handle, nullptr);
}

void RvGraphicsPipeline::create()
{
	//Shader Stage creation (assign shader modules to shader stages in the pipeline).
	const uint32_t modulesCount = attachedShaderModules.size();
	vector<VkPipelineShaderStageCreateInfo> shaderStagesCreateInfos(modulesCount);
	for (uint32_t i = 0; i < modulesCount; i++)
	{
		VkPipelineShaderStageCreateInfo shaderStageCreateInfo = {};
		shaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStageCreateInfo.stage = attachedShaderStageBits[i];
		shaderStageCreateInfo.module = *attachedShaderModules[i];
		shaderStageCreateInfo.pName = shaderEntryPoints[i];
		shaderStagesCreateInfos.push_back(shaderStageCreateInfo);
	}

	//Pipeline Layout (Specify the uniform variables used in the pipeline)
	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
	pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
	pipelineLayoutInfo.pushConstantRangeCount = static_cast<uint32_t>(pushConstantRanges.size());
	pipelineLayoutInfo.pPushConstantRanges = pushConstantRanges.data();

	if (vkCreatePipelineLayout(device->handle, &pipelineLayoutInfo, nullptr, &layout) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create pipeline layout!");
	}

	VkGraphicsPipelineCreateInfo createInfo;
	createInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

	//Set External References
	createInfo.renderPass = renderPass.handle;
	createInfo.basePipelineHandle = basePipeline;
	createInfo.flags = createFlags;
	createInfo.subpass = subpass;

	//Internal States
	createInfo.stageCount = static_cast<uint32_t>(shaderStagesCreateInfos.size());
	createInfo.pStages = shaderStagesCreateInfos.data();
	createInfo.layout = layout;
	vertexInputState.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexAttributeDescriptions.size());
	vertexInputState.pVertexAttributeDescriptions = vertexAttributeDescriptions.data();
	vertexInputState.vertexBindingDescriptionCount = static_cast<uint32_t>(vertexBindingDescriptions.size());
	vertexInputState.pVertexBindingDescriptions = vertexBindingDescriptions.data();
	createInfo.pVertexInputState = &vertexInputState;
	createInfo.pInputAssemblyState = &inputAssemblyState;
	createInfo.pTessellationState = &tessellationState;
	viewportState.viewportCount = static_cast<uint32_t>(viewports.size());
	viewportState.pViewports = viewports.data();
	viewportState.scissorCount = static_cast<uint32_t>(scissors.size());
	viewportState.pScissors = scissors.data();
	createInfo.pViewportState = &viewportState;
	createInfo.pRasterizationState = &rasterizationState;
	multisampleState.pSampleMask = &sampleMask;
	createInfo.pMultisampleState = &multisampleState;
	createInfo.pDepthStencilState = &depthStencilState;
	colorBlendState.attachmentCount = static_cast<uint32_t>(colorBlendAttachments.size());
	colorBlendState.pAttachments = colorBlendAttachments.data();
	createInfo.pColorBlendState = &colorBlendState;
	dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
	dynamicState.pDynamicStates = dynamicStates.data();
	createInfo.pDynamicState = &dynamicState;
	VkResult result = vkCreateGraphicsPipelines(device->handle, pipelineCache, 1, &createInfo, nullptr, &handle);
	if (result != VK_SUCCESS)
	{
		throw std::exception("Could not create Graphics Pipeline!");
	}
}

void RvGraphicsPipeline::recreate()
{

}

void RvGraphicsPipeline::attachShader(const VkShaderStageFlagBits stageFlagBits, const VkShaderModule* shaderModule, const char* entryPointName)
{
	if ((stageFlagBits & allAttachedShaderStageBits) > 0)
	{
		throw std::exception("A Shader is already attached with this stage(s).");
	}

	forceAttachShader(stageFlagBits, shaderModule, entryPointName);
}

void RvGraphicsPipeline::forceAttachShader(const VkShaderStageFlagBits stageFlagBits, const VkShaderModule* shaderModule, const char* entryPointName)
{
	allAttachedShaderStageBits = static_cast<VkShaderStageFlagBits>(
		static_cast<uint32_t>(allAttachedShaderStageBits)
		| static_cast<uint32_t>(stageFlagBits)	//Perform OR operation to add new bits
		);
	attachedShaderStageBits.push_back(stageFlagBits);
	attachedShaderModules.push_back(shaderModule);
	shaderEntryPoints.push_back(entryPointName);
}

void RvGraphicsPipeline::detachShader(const VkShaderModule * shaderModule)
{
	int32_t i = 0;
	vector<const VkShaderModule*>::const_iterator mIt = nullptr;
	vector<VkShaderStageFlagBits>::const_iterator sIt = nullptr;
	vector<const char*>::const_iterator eIt = nullptr;
	for (mIt = attachedShaderModules.begin(), sIt = attachedShaderStageBits.begin(), eIt = shaderEntryPoints.begin();
		mIt != attachedShaderModules.end(); mIt++, sIt++, eIt++)
	{
		if (*mIt == shaderModule)
		{
			allAttachedShaderStageBits = static_cast<VkShaderStageFlagBits>(
				static_cast<uint32_t>(allAttachedShaderStageBits)&	//Mask Value with
				~static_cast<uint32_t>(attachedShaderStageBits[i])	//negated use values
				);
			attachedShaderStageBits.erase(sIt);
			attachedShaderModules.erase(mIt);
			shaderEntryPoints.erase(eIt);
			return;
		}
		i++;
	}

	fmt::print(stdout, "Given ShaderModule is not attached!");
}

RvGraphicsPipeline* RvGraphicsPipeline::defaultGraphicsPipeline(RvDevice& device, const RvRenderPass& renderPass)
{
	static RvGraphicsPipeline* defaultPipeline = nullptr;
	if (defaultPipeline == nullptr)
	{
		defaultPipeline = new RvGraphicsPipeline(&device);
		defaultPipeline->renderPass = renderPass;
		
		//ShaderModules
		vector<char> vertexCode = rvTools::readFile("../data/shaders/skinned_tex_color.vert");
		vector<char> vertexShader = rvTools::compileShaderText("Polygon Vertex Shader", vertexCode,
			shaderc_shader_kind::shaderc_vertex_shader, "main");
		VkShaderModule vertexModule = rvTools::createShaderModule(device.handle, vertexShader);
		defaultPipeline->attachShader(VK_SHADER_STAGE_VERTEX_BIT, &vertexModule, "main");
		vector<char> fragmentCode = rvTools::readFile("../data/shaders/phong_tex_color.frag");
		vector<char> fragmentShader = rvTools::compileShaderText("Polygon Fragment Shader", fragmentCode,
			shaderc_shader_kind::shaderc_fragment_shader, "main");
		VkShaderModule fragModule = rvTools::createShaderModule(device.handle, fragmentShader);
		defaultPipeline->attachShader(VK_SHADER_STAGE_FRAGMENT_BIT, &fragModule, "main");
	}
	return defaultPipeline;
}

VkPipelineCache RvGraphicsPipeline::defaultPipelineCache(RvDevice& device)
{
	static VkPipelineCache pipelineCache = VK_NULL_HANDLE;
	if (pipelineCache == VK_NULL_HANDLE)
	{
		VkPipelineCacheCreateInfo cacheCreateInfo = {};
		cacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;

		//Try loading cache from disk
		try
		{
			vector<char> data = rvTools::readFile("..\data\cache\default.pipeline");
			cacheCreateInfo.initialDataSize = data.size();
			cacheCreateInfo.pInitialData = data.data();
		}
		catch (std::exception exc)
		{
			fmt::print(stdout, "No default PipelineCache file found, creating new one...");
		}

		//Create cache object
		const VkResult result = vkCreatePipelineCache(device.handle, &cacheCreateInfo, nullptr, &pipelineCache);

		//Error handling
		if (result != VK_SUCCESS)
		{
			throw std::exception("Failed to create default Pipeline Cache!");
		}
	}
	return pipelineCache;
}

RvGraphicsPipeline::operator VkPipeline() const
{
	return handle;
}
