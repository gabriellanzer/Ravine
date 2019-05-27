#include "RvTime.h"

std::chrono::high_resolution_clock::time_point RvTime::startTime;
std::chrono::high_resolution_clock::time_point RvTime::lastFrameTime;
std::chrono::high_resolution_clock::time_point RvTime::currentFrameTime;
double RvTime::_deltaTime;

void RvTime::initialize()
{
	startTime = std::chrono::high_resolution_clock::now();
	lastFrameTime = currentFrameTime = startTime;
}

void RvTime::update()
{
	lastFrameTime = currentFrameTime;
	currentFrameTime = std::chrono::high_resolution_clock::now();
	_deltaTime = std::chrono::duration<double, std::chrono::seconds::period>(currentFrameTime - lastFrameTime).count();
}

double RvTime::elapsedTime()
{
	return std::chrono::duration<double, std::chrono::seconds::period>(currentFrameTime - startTime).count();
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
