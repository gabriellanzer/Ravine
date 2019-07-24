#include "RvTime.h"

//ImGui Includes
#include "imgui.h"

eastl::chrono::high_resolution_clock::time_point RvTime::startTime;
eastl::chrono::high_resolution_clock::time_point RvTime::lastFrameTime;
eastl::chrono::high_resolution_clock::time_point RvTime::currentFrameTime;
double RvTime::internalDeltaTime;
double RvTime::internalElapsedTime;
int RvTime::internalFps;
int RvTime::timeIt;
double RvTime::times[100];

void RvTime::initialize()
{
	startTime = eastl::chrono::high_resolution_clock::now();
	lastFrameTime = currentFrameTime = startTime;
	timeIt = 0;
}

void RvTime::update()
{
	lastFrameTime = currentFrameTime;
	currentFrameTime = eastl::chrono::high_resolution_clock::now();
	internalDeltaTime = eastl::chrono::duration<double, eastl::chrono::seconds::period>(currentFrameTime - lastFrameTime).count();
	internalElapsedTime = eastl::chrono::duration<double, eastl::chrono::seconds::period>(currentFrameTime - startTime).count();
	times[timeIt] = internalDeltaTime;
	timeIt = (timeIt + 1) % 100;
	static double totalTime = 0;
	totalTime = 0;
	for (double time : times)
	{
		totalTime += time * 0.01;
	}
	internalFps = static_cast<int>(1.0 / totalTime);
}

double RvTime::elapsedTime()
{
	return internalElapsedTime;
}

double RvTime::deltaTime()
{
	return internalDeltaTime;
}

int RvTime::framesPerSecond()
{
	return internalFps;
}

RvTime::RvTime() = default;

RvTime::~RvTime() = default;
