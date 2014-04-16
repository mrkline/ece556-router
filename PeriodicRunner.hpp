#ifndef __PERIODIC_RUNNER_HPP__
#define __PERIODIC_RUNNER_HPP__

#include <chrono>
#include <mutex>

template <typename T = std::chrono::seconds>
class PeriodicRunner {

public:

	template <typename C>
	PeriodicRunner(C dur) : duration(dur) { }

	template <typename R>
	void runPeriodically(R toRun)
	{
		std::lock_guard<std::mutex> lg(runMutex);
		if (Clock::now() >= lastRan + duration)
		{
			toRun();
			lastRan = Clock::now();
		}
	}


private:

	typedef std::chrono::high_resolution_clock Clock;

	Clock::time_point lastRan;
	std::mutex runMutex;
	T duration;
};

#endif
