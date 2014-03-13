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
