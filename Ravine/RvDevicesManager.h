#ifndef RV_DEVICES_MANAGER_H
#define RV_DEVICES_MANAGER_H

#include <eastl/hash_map.h>
using eastl::hash_map;

#include "RvDevice.h"

class RvDevicesManager
{
private:
	vector<RvDevice> devices;
	hash_map<string, RvDevice*> deviceNameMap;
	
public:
	static RvDevice* current;

	static void initializer(VkInstance& instance);
	static void detectCurrentDevice();
	static void loadDevicesList();
	static void setCurrentDevice(string deviceName);
	static vector<string> getDevicesNames();
	static const RvDevice* getCurrentDevice(string deviceName);
};

#endif
