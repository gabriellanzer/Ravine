#ifndef RV_CONFIG_H
#define RV_CONFIG_H

//EASTL Includes
#include <EASTL/vector.h>
using eastl::vector;

//Vulkan Includes
#include <vulkan\vulkan.h>

namespace rvCfg
{
	//Validation layers to be enabled
	const vector<const char*> ValidationLayers = {
		"VK_LAYER_LUNARG_standard_validation"
	};

	//Physical Device required extensions
	const vector<const char*> DeviceExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

	bool CheckValidationLayerSupport();
}


#ifndef NDEBUG
#define	VALIDATION_LAYERS_ENABLED
#endif

#endif
