
#ifndef COLORMAP_HPP_YZ1X1P
#define COLORMAP_HPP_YZ1X1P

#include <array>
#include <ostream>
#include <cstdio>

struct RGB {
	uint8_t r, g, b;
};

inline std::ostream &operator <<(std::ostream &os, const RGB &rgb)
{
	using namespace std;
	array<char, 8> buffer;
	snprintf(buffer.data(), buffer.size(), "#%02hhx%02hhx%02hhx", rgb.r, rgb.g, rgb.b);
	return os << buffer.data();
}


extern std::array<RGB, 255> matplotlibHotColormap;

#endif // COLORMAP_HPP_YZ1X1P