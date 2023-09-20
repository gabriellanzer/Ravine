#ifndef RV_CONFIG_H
#define RV_CONFIG_H

//STD Includes
#include "RvStdDefs.h"

//Vulkan Includes
#include "volk.h"

namespace rvCfg
{
	//Validation layers to be enabled
	const vector<const char*> VALIDATION_LAYERS = {
		"VK_LAYER_KHRONOS_validation"
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
