#ifndef __PERIODIC_RUNNER_HPP__
#define __PERIODIC_RUNNER_HPP__

#include <chrono>

template <typename T = std::chrono::seconds>
class PeriodicRunner {

public:

	template <typename C>
	PeriodicRunner(C dur) : duration(dur) { }

	template <typename R>
	void runPeriodically(R toRun)
	{
		if (Clock::now() >= lastRan + duration)
		{
			toRun();
			lastRan = Clock::now();
		}
	}


private:

	typedef std::chrono::high_resolution_clock Clock;

	Clock::time_point lastRan;
	T duration;
};

#endif
