#include "RvGraphicsPipeline.h"

#include <fmt/printf.h>

RvGraphicsPipeline::RvGraphicsPipeline(VkPipeline basePipelineHandle, int32_t basePipelineIndex) : 
	basePipelineHandle(basePipelineHandle), basePipelineIndex(basePipelineIndex)
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
	

}

void RvGraphicsPipeline::recreate()
{

}

void RvGraphicsPipeline::attachShader(const VkShaderStageFlagBits stageFlagBits, const VkShaderModule* shaderModule, const char* entryPointName)
{
	if((stageFlagBits & allAttachedShaderStageBits) > 0)
	{
		throw std::exception("A Shader is already attached with this stage(s).");
	}

	forceAttachShader(stageFlagBits, shaderModule, entryPointName);
}

void RvGraphicsPipeline::forceAttachShader(const VkShaderStageFlagBits stageFlagBits, const VkShaderModule* shaderModule, const char* entryPointName)
{
	allAttachedShaderStageBits = static_cast<VkShaderStageFlagBits>(
		static_cast<uint32_t>(allAttachedShaderStageBits)
		|static_cast<uint32_t>(stageFlagBits)	//Perform OR operation to add new bits
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
		if(*mIt == shaderModule)
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

	fmt::print(stdout, "Given ShaderModule is attached.");
}

RvGraphicsPipeline::operator VkPipeline() const
{
	return handle;
}
