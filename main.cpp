// ECE556 - Copyright 2014 University of Wisconsin-Madison.  All Rights Reserved.

#include "ece556.hpp"

// I prefer printf to cout. It's easier to format stuff and the stream operator for cout can be weird.
#include <cstdio>

int main(int argc, char** argv)
{

 	if (argc!=3) {
 		printf("Usage : route <input_benchmark_name> <output_file_name> \n");
 		return 1;
 	}

	const char* inputFileName = argv[1];
 	const char* outputFileName = argv[2];

 	/// create a new routing instance
 	RoutingInst rst;

 	/// read benchmark
	try {
		readBenchmark(inputFileName, rst);

		/// run actual routing
		solveRouting(rst);

		/// write the result
		writeOutput(outputFileName, rst);

		printf("\nDONE!\n");
		return 0;
	}
	catch (std::exception& ex) {
		printf("Error: %s\n", ex.what());
		return 1;
	}
}
