#pragma once

#include <cassert>
#include <iterator>
#include <vector>

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
