#pragma once

//EASTL Includes
#include <eastl/chrono.h>

/**
 * \brief Class that handles high resolution time measurements.
 */
class RvTime
{
public:

	/**
	 * \brief Initializes the high precision clock instances.
	 */
	static void initialize();

	/**
	 * \brief Calculate all the time measurements with a high precision clock.
	 */
	static void update();

	/**
	 * \brief Gets the calculated elapsed time since the application started.
	 * \return Time in seconds.
	 */
	static double elapsedTime();

	/**
	 * \brief Gets the calculated time variation (delta) since last frame.
	 * \return Time in seconds.
	 */
	static double deltaTime();

	/**
	 * \brief Gets the calculated FPS based on the last application frames.
	 * \return Amount of frames.
	 */
	static int framesPerSecond();

private:
	RvTime();
	~RvTime();
	
	/**
	 * \brief All latest measurements that will be used to calculate FPS.
	 */
	static double times[100];

	/**
	 * \brief Iterator of the current time measurement being updated.
	 */
	static int timeIt;

	/**
	 * \brief Internal value for the frames per second.
	 */
	static int internalFps;

	/**
	 * \brief Internal value for the delta time.
	 */
	static double internalDeltaTime;

	/**
	 * \brief Internal value for the elapsed time.
	 */
	static double internalElapsedTime;

	
	/**
	 * \brief High resolution time point for the start of the clock.
	 */
	static eastl::chrono::high_resolution_clock::time_point startTime;

	/**
	 * \brief High resolution time point of the last frame start.
	 */
	static eastl::chrono::high_resolution_clock::time_point lastFrameTime;

	/**
	 * \brief High resolution time point of the current frame.
	 */
	static eastl::chrono::high_resolution_clock::time_point currentFrameTime;
};
