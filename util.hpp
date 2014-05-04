#ifndef UTIL_HPP_GH34WZ
#define UTIL_HPP_GH34WZ

#include <vector>
#include <ostream>
#include <utility>     // declval
#include <unordered_map>

template <class K, typename V>
V &getElementInsertingIfNecessary(std::unordered_map<K, V> &map, const K& key, const V& default_)
{
	auto it = map.find(key);
	if (it == std::end(map)) {
		V& ret = map[key];
		ret = default_;
		return ret;
	}
	else {
		return it->second;
	}
}

template <typename K, typename V>
V getElementOrDefault(const std::unordered_map<K, V>& map, const K& key, const V& default_)
{
	auto it = map.find(key);
	if (it == std::end(map))
		return default_;
	else
		return it->second;
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
