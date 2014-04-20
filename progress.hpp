
#ifndef PROGRESS_HPP_753J6J
#define PROGRESS_HPP_753J6J

#include <iostream>
#include <utility>
class ProgressBar
{
	void drawBar()
	{
		out << "[";
		const unsigned stop = value * width / max;
		for(unsigned i = 0; i < width; ++i) {
			out << (i <= stop? '*' : ' ');
		}
		out << "] " << std::min<int>(100, value * 100 / max + 1) << "\n";
		endl();
	}

	unsigned _auxLines = 0;

	template <typename T>
	void write(const T &t)
	{
		out << t;
	}

	template <typename F, typename... R>
	void write(const F &f, const R &...r)
	{
		write(f);
		write(r...);
	}
public:
	unsigned width = 70, value = 0, max = 100;
	std::ostream &out;

	ProgressBar(std::ostream &out)
	: out(out)
	{ }

	void endl()
	{
		_auxLines++;
		out << "\n\033[0K";
	}

	template <typename... Args>
	ProgressBar &writeln(const Args &...args)
	{
		write(args...);
		endl();
		return *this;
	}

	ProgressBar &draw()
	{
		resetCursor();
		drawBar();
		return *this;
	}

	void resetCursor()
	{
		out << "\033[" << (_auxLines+1) << "A" << "\033[1G"; 
		_auxLines = 0;
		endl();
	}
};

#endif // PROGRESS_HPP_753J6J

