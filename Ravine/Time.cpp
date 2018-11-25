#include "Time.h"

std::chrono::high_resolution_clock::time_point Time::startTime;
std::chrono::high_resolution_clock::time_point Time::lastFrameTime;
std::chrono::high_resolution_clock::time_point Time::currentFrameTime;
double Time::_deltaTime;

void Time::initialize()
{
	startTime = std::chrono::high_resolution_clock::now();
	lastFrameTime = currentFrameTime = startTime;
}

void Time::update()
{
	lastFrameTime = currentFrameTime;
	currentFrameTime = std::chrono::high_resolution_clock::now();
	_deltaTime = std::chrono::duration<double, std::chrono::seconds::period>(currentFrameTime - lastFrameTime).count();
}

double Time::elapsedTime()
{
	return std::chrono::duration<double, std::chrono::seconds::period>(currentFrameTime - startTime).count();
}

double Time::deltaTime()
{
	return _deltaTime;
}

Time::Time()
{
}


Time::~Time()
{
}
