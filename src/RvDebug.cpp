
//FMT Includes
#include <fmt/printf.h>

//Ravine Includes
#include "RvDebug.h"
#include "RvTools.h"

namespace rvDebug
{

	//Callback function
	VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objType, uint64_t obj, size_t location, int32_t code, const char* layerPrefix, const char* msg, void* userData) {
		fmt::print(stderr, "Validation layer: {0}\n", msg);

		return VK_FALSE;
	}

	//Creating debug callback
	VkResult createDebugReportCallbackExt(VkInstance instance, const VkDebugReportCallbackCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugReportCallbackEXT* pCallback) {
		auto func = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT");
		if (func != nullptr) {
			return func(instance, pCreateInfo, pAllocator, pCallback);
		}
		else {
			return VK_ERROR_EXTENSION_NOT_PRESENT;
		}
	}

	//Setting up debug callback
	void setupDebugCallback(VkInstance instance)
	{
		VkDebugReportCallbackCreateInfoEXT createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
		createInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
		createInfo.pfnCallback = debugCallback;

		if (createDebugReportCallbackExt(instance, &createInfo, nullptr, &callback) != VK_SUCCESS) {
			throw std::runtime_error("Failed to set up debug callback!");
		}
	}

	//Destroy handle
	void destroyDebugReportCallbackExt(VkInstance instance, VkDebugReportCallbackEXT callback, const VkAllocationCallbacks* pAllocator) {
		if (callback != NULL) {
			auto func = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT");
			if (func != nullptr) {
				func(instance, callback, pAllocator);
			}
		}
	}

	void destroyDebugCallback(VkInstance instance)
	{
		destroyDebugReportCallbackExt(instance, callback, nullptr);
	}

};
