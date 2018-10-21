#include "Time.h"

std::chrono::high_resolution_clock::time_point Time::startTime;
std::chrono::high_resolution_clock::time_point Time::lastFrameTime;
std::chrono::high_resolution_clock::time_point Time::currentFrameTime;

void Time::initialize()
{
	startTime = std::chrono::high_resolution_clock::now();
	lastFrameTime = currentFrameTime = startTime;
}

void Time::update()
{
	lastFrameTime = currentFrameTime;
	currentFrameTime = std::chrono::high_resolution_clock::now();
}

double Time::elapsedTime()
{
	return std::chrono::duration<double, std::chrono::seconds::period>(currentFrameTime - startTime).count();
}

double Time::deltaTime()
{
	return std::chrono::duration<double, std::chrono::seconds::period>(currentFrameTime - lastFrameTime).count();
}

Time::Time()
{
}


Time::~Time()
{
}
