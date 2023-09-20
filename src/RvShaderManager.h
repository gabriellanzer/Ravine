#ifndef RV_SHADER_MANAGER_H
#define RV_SHADER_MANAGER_H

#include <eastl/hash_map.h>
#include <eastl/string.h>
#include <eastl/vector.h>
using eastl::hash_map;
using eastl::string;
using eastl::vector;

#include <fmt/printf.h>
#include "RvTools.h"

struct RvShaderModule
{
	VkShaderStageFlagBits stageFlagBits = static_cast<VkShaderStageFlagBits>(0);
	string entryPoint					= "main";
	string shaderName					= "";
	string filePath						= "";
	VkShaderModule handle				= VK_NULL_HANDLE;

	void recompile();
	void destroy();
};

class RvShaderManager
{
private:
	RvShaderManager();

public:
	static hash_map<string, RvShaderModule*> shaderModulesMap;
	static hash_map<string, uint32_t> shaderSourceHashes;
	static vector<RvShaderModule*> shaderModules;

	static const RvShaderModule* createShader(string filePath, string shaderName, VkShaderStageFlagBits shaderStageFlagBits, string entryPoint);
	static const RvShaderModule* getShader(string shaderName);
	static const RvShaderModule* recompileShader(string shaderName);
};


#endif
