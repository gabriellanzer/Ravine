#pragma once

//EASTL Includes
#include <EASTL/chrono.h>

//GLFW Include

class RvTime
{
public:

	static eastl::chrono::high_resolution_clock::time_point startTime;
	static eastl::chrono::high_resolution_clock::time_point lastFrameTime;
	static eastl::chrono::high_resolution_clock::time_point currentFrameTime;

	static void initialize();
	static void update();

	static double elapsedTime();
	static double deltaTime();
	
private:
	RvTime();
	~RvTime();

	static double _deltaTime;
};
