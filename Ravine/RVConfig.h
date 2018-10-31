#ifndef RV_CONFIG_H
#define RV_CONFIG_H

//STD Includes
#include <vector>

//Vulkan Includes
#include <vulkan\vulkan.h>

//Validation layers to be enabled
const std::vector<const char*> rvCfgValidationLayers = {
	"VK_LAYER_LUNARG_standard_validation"
};

//Physical Device required extensions
const std::vector<const char*> rvCfgDeviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

#ifndef NDEBUG
#define	VALIDATION_LAYERS_ENABLED
#endif

#endif
