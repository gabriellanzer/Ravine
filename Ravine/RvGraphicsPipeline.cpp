#include "RvGraphicsPipeline.h"

#include <fmt/printf.h>

RvGraphicsPipeline::RvGraphicsPipeline()
{
}

RvGraphicsPipeline::~RvGraphicsPipeline()
{
	vkDestroyPipelineLayout(device->handle, layout, nullptr);
	vkDestroyPipeline(device->handle, handle, nullptr);
}

void RvGraphicsPipeline::create()
{

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

}

void RvGraphicsPipeline::deattachShader(const VkShaderModule * shaderModule)
{
	for (uint32_t i = 0; i < attachedShaderModules.size(); i++)
	{
		if(attachedShaderModules[i] == shaderModule)
		{
			allAttachedShaderStageBits = static_cast<VkShaderStageFlagBits>(
				static_cast<uint32_t>(allAttachedShaderStageBits)^static_cast<uint32_t>(attachedShaderStageBits[i])
			);
			return;
		}
	}

	fmt::print(stdout, "");
}

RvGraphicsPipeline::operator VkPipeline() const
{
	return handle;
}
