#pragma once

#include <cassert>
#include <iterator>
#include <vector>
#include <algorithm>
#include <future>
/**
 * \brief Partitions an iterable collection into equally-ish sized partitions
 *
 * Returns begin and end, even though they are already known, for convenience.
 */
template <typename InputIt>
std::vector<InputIt> partitionCollection(InputIt begin, InputIt end, size_t numPartitions)
{
	assert(numPartitions > 1); // Don't be stupid

	const size_t step = std::distance(begin, end) / numPartitions;

	std::vector<InputIt> ret(numPartitions + 1);
	ret[0] = begin;
	ret[numPartitions] = end;
	for (size_t i = 1; i < numPartitions; ++i)
		ret[i] = ret[i - 1] + step;

	return ret;
}

/// Perform a function for each element in a collection in a number of threads equal to
/// the number of hardware threads available.
template <typename I, typename F>
void parallelForEach(I begin, I end, const F &function)
{
	auto parts = partitionCollection(
		begin, 
		end, 
		std::max(1u, std::thread::hardware_concurrency())
	);
	

	std::vector<std::future<void>> futures;

	for(auto it = parts.begin() + 1, end = parts.end(); it != end; ++it) {
		futures.emplace_back(std::async([&function, it] {
			std::for_each(*(it - 1), *it, function);
		}));
	}

	for(auto &f : futures) f.wait();
}
