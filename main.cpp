// ECE556 - Copyright 2014 University of Wisconsin-Madison.  All Rights Reserved.

#include "ece556.hpp"
#include "reader.hpp"

// I prefer printf to cout. It's easier to format stuff and the stream operator for cout can be weird.
#include <cstdio>
#include <iostream>

static void testReadingInput()
{
	try {
		auto inst = readRoutingInst(std::cin);

		std::cout << "read " << inst.nets.size() << " nets\n";
	
		for(const auto &net : inst.nets) {
			std::cout << "-> net n" << net.id << " has " << net.pins.size() << " pins.\n";

			for(const auto &point : net.pins) {
				std::cout << "--> pin: " << point.x << " " << point.y << "\n";
			}
		}
	}
	catch(std::exception &exc) {
		std::cerr << "Error: " << exc.what() << "\n";
	}
}

int main(int argc, char** argv)
{
	static_cast<void>(testReadingInput); // suppress unused warning
// 	testReadingInput(); return 0;


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
