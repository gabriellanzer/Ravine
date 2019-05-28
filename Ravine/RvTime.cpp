#include "RvTime.h"

eastl::chrono::high_resolution_clock::time_point RvTime::startTime;
eastl::chrono::high_resolution_clock::time_point RvTime::lastFrameTime;
eastl::chrono::high_resolution_clock::time_point RvTime::currentFrameTime;
double RvTime::_deltaTime;

void RvTime::initialize()
{
	startTime = eastl::chrono::high_resolution_clock::now();
	lastFrameTime = currentFrameTime = startTime;
}

void RvTime::update()
{
	lastFrameTime = currentFrameTime;
	currentFrameTime = eastl::chrono::high_resolution_clock::now();
	_deltaTime = eastl::chrono::duration<double, eastl::chrono::seconds::period>(currentFrameTime - lastFrameTime).count();
}

double RvTime::elapsedTime()
{
	return eastl::chrono::duration<double, eastl::chrono::seconds::period>(currentFrameTime - startTime).count();
}

double RvTime::deltaTime()
{
	return _deltaTime;
}

RvTime::RvTime()
{
}


RvTime::~RvTime()
{
}
