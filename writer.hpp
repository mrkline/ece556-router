#ifndef WRITER_HPP_6W2E2N
#define WRITER_HPP_6W2E2N
#include "RoutingSolver.hpp"

struct Writer
{
	std::ostream &out;
	const RoutingSolver &routing;
	
	Writer(std::ostream &out, const RoutingSolver &routing)
	: out(out)
	, routing(routing)
	{ }
	
	void write(const Point &);
	void write(const Net &);
	void write(const Edge &);
	void writeRouting();
};

inline void write(std::ostream &out, const RoutingSolver &routing)
{
	Writer(out, routing).writeRouting();
}

#endif // WRITER_HPP_6W2E2N