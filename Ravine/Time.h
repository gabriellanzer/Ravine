#pragma once

//STD Includes
#include <chrono>

class Time
{
public:

	static std::chrono::high_resolution_clock::time_point startTime;
	static std::chrono::high_resolution_clock::time_point lastFrameTime;
	static std::chrono::high_resolution_clock::time_point currentFrameTime;

	static void initialize();
	static void update();

	static double elapsedTime();
	static double deltaTime();


private:
	Time();
	~Time();
};
