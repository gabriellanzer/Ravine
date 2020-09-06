#ifndef RV_DEBUG_H
#define RV_DEBUG_H

//Vulkan Include
#include "volk.h"

namespace rvDebug
{
	//Debug callback handler
	static VkDebugReportCallbackEXT callback;

	VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
		VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objType, uint64_t obj,
		size_t location, int32_t code, const char* layerPrefix, const char* msg, void* userData);

	VkResult createDebugReportCallbackExt(
		VkInstance instance, const VkDebugReportCallbackCreateInfoEXT* pCreateInfo,
		const VkAllocationCallbacks* pAllocator, VkDebugReportCallbackEXT* pCallback);

	void destroyDebugReportCallbackExt(
		VkInstance instance, VkDebugReportCallbackEXT callback,
		const VkAllocationCallbacks* pAllocator);

	//Create DebugReport callback handler and check validation layer support
	void setupDebugCallback(VkInstance instance);

	//Destroy DebugReport callback handler (if any)
	void destroyDebugCallback(VkInstance instance);

};

#endif