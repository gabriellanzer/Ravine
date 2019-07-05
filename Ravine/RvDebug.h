#ifndef RV_DEBUG_H
#define RV_DEBUG_H

//Vulkan Include
#include <vulkan/vulkan.h>

namespace rvDebug
{

	//Debug callback handler
	static VkDebugReportCallbackEXT callback;

	VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
		VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objType, uint64_t obj,
		size_t location, int32_t code, const char* layerPrefix, const char* msg, void* userData);

	VkResult CreateDebugReportCallbackEXT(
		VkInstance instance, const VkDebugReportCallbackCreateInfoEXT* pCreateInfo,
		const VkAllocationCallbacks* pAllocator, VkDebugReportCallbackEXT* pCallback);

	//Create DebugReport callback handler and check validation layer support
	void setupDebugCallback(VkInstance instance);

	void destroyDebugReportCallbackExt(VkInstance instance, VkDebugReportCallbackEXT callback, const VkAllocationCallbacks* pAllocator);
};

#endif