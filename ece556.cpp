// ECE556 - Copyright 2014 University of Wisconsin-Madison.  All Rights Reserved.

#include <fstream>

#include "ece556.hpp"
#include "reader.hpp"

void readBenchmark(const char *fileName, RoutingInst& rst)
{
	std::ifstream in(fileName);
	if(!in) {
		throw std::runtime_error("I/O Error");
	}
	rst = readRoutingInst(in);
}

void solveRouting(RoutingInst& rst)
{
  /*********** TO BE FILLED BY YOU **********/
}

void writeOutput(const char *outRouteFile, RoutingInst& rst)
{
  /*********** TO BE FILLED BY YOU **********/
}


std::ostream &operator <<(std::ostream &os, const Point &p)
{
	return os << "Point{" << p.x << ", " << p.y << "}";
}

std::ostream &operator <<(std::ostream &os, const Edge &e)
{
	return os << "Edge{" << e.p1 << ", " << e.p2 << "}";
}
