#ifndef UTIL_HPP_GH34WZ
#define UTIL_HPP_GH34WZ

#include <vector>
#include <ostream>
#include <utility>     // declval
#include <type_traits> // remove_reference

template <class T>
T &getElementResizingIfNecessary(std::vector<T> &vec, size_t index, const T &default_)
{
	size_t vecSize = vec.size();
	if(index >= vecSize) {
		vec.resize(index + 1);
		for(size_t i = vecSize; i < vec.size(); ++i) {
			vec[i] = default_;
		}
	}

	return vec[index];
}

template <class Collection, 
          class T=typename Collection::value_type>
T getElementOrDefault(const Collection &vec, size_t index, const T &default_)
{
	size_t vecSize = vec.size();
	if(index >= vecSize) {
		return default_;
	}
	else {
		return vec[index];
	}
}


template <class T>
std::ostream &operator <<(std::ostream &os, const std::vector<T> &vec)
{
	os << "{";

	for(auto it = vec.begin(), begin = vec.begin(), end = vec.end();
			it != vec.end(); 
			++it) {

		if(it != vec.begin()) {
			os << ", ";
		}

		os << *it;
	}
	os << "}";

	return os;
}

#endif // UTIL_HPP_GH34WZ