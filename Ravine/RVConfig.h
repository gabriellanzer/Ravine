#ifndef RV_CONFIG_H
#define RV_CONFIG_H

//EASTL Includes
#include <eastl/vector.h>
using eastl::vector;

//Vulkan Includes
#include <vulkan\vulkan.h>

namespace rvCfg
{
	//Validation layers to be enabled
	const vector<const char*> VALIDATION_LAYERS = {
		"VK_LAYER_LUNARG_standard_validation"
	};

	//Physical Device required extensions
	const vector<const char*> DEVICE_EXTENSIONS = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

	bool checkValidationLayerSupport();
}


#ifndef NDEBUG
#define	VALIDATION_LAYERS_ENABLED
#endif

#endif
