// ECE556 - Copyright 2014 University of Wisconsin-Madison.  All Rights Reserved.

#include "ece556.hpp"
#include "RoutingSolver.hpp"
#include "reader.hpp"
#include "writer.hpp"
#include "colormap.hpp"


// I prefer printf to cout. It's easier to format stuff and the stream operator for cout can be weird.
#include <cstdio>
#include <iostream>
#include <getopt.h> 
#include <cstring>


struct Options {
	std::string inputBenchmark, outputFile;

	bool useNetDecomposition = true;
	bool useNetOrdering = true;
	bool emitSVG = false;
};



static bool optArgToBool(const char *name)
{
	if(std::strcmp(optarg, "1") == 0 || std::strcmp(optarg, "=1") == 0) {
		return true;
	}
	else if(std::strcmp(optarg, "0") == 0 || std::strcmp(optarg, "=0") == 0) {
		return false;
	}
	else {
		std::cerr << "Expected either \"1\" or \"0\" for option " << name << "\n";
		std::exit(1);
	}
}

// [[noreturn]]
static void usage(int argc, char **argv)
{
	std::cerr << "Usage: " << (argc > 0? argv[0] : "route") << " [-d=0] [-n=0] INPUT_BENCHMARK OUTPUT\n";
	std::exit(1);
}

static Options parseOpts(int argc, char **argv)
{
	int ch;
	Options result;
	opterr = 0;

	option longopts[] = {
		{"help", no_argument, nullptr, 'h'},
		{"decomp", required_argument, nullptr, 'd'},
		{"order", required_argument, nullptr, 'n'},
		{"emit-svg", no_argument, nullptr, 's'},
		{nullptr, 0, nullptr, 0}
	};

	while((ch = getopt_long(argc, argv, "hd:n:s", longopts, nullptr)) != -1) {
		switch(ch) {
			case 'd': {
				result.useNetDecomposition = optArgToBool("-d");
			} break;
			case 'n': {
				result.useNetOrdering = optArgToBool("-n");
			} break;
			case 'h': {
				usage(argc, argv);
			} break;
			case 's': {
				result.emitSVG = true;
			} break;
			case ':': break;
			default: {
				std::cerr << "Unrecognized option: " << char(ch) << "\n";
				usage(argc, argv);
			} break;
		}
	}

	int remainingArgCount = argc - optind;
	char **remainingArgs = argv + optind;

	if(remainingArgCount != 2) {
		usage(argc, argv);
	}
	else {
		result.inputBenchmark = remainingArgs[0];
		result.outputFile = remainingArgs[1];
	}

	return result;
}

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

		int i = 0;
		for(int cap : inst.edgeCaps) {
			std::cout << "-> edge " << i << " cap " << cap << "\n";
			++i;
		}
	}
	catch(std::exception &exc) {
		std::cerr << "Error: " << exc.what() << "\n";
	}
}

namespace {

std::string strerrno()
{
	return strerror(errno);
}


RoutingInst readRoutingInstFromPath(const std::string &path)
{
	std::ifstream in(path);
	if(!in)
	{
		throw std::runtime_error("Couldn't open file: " + strerrno());
	}
	return readRoutingInst(in);
}

void writeToPath(const std::string &path, const RoutingInst &inst)
{
	std::ofstream out(path);
	if(!out)
	{
		throw std::runtime_error("Couldn't open output file: " + strerrno());
	}

	write(out, inst);

	out.close();

	if(!out)
	{
		throw std::runtime_error("Couldn't close output file: " + strerrno());
	}
}

} // end anonymous namespace


int main(int argc, char** argv)
{
	static_cast<void>(testReadingInput); // suppress unused warning

	auto opts = parseOpts(argc, argv);

 	/// create a new routing instance

 	/// read benchmark
	try {
		auto problem = readRoutingInstFromPath(opts.inputBenchmark);
		RoutingSolver rst(problem);

		rst.useNetDecomposition = opts.useNetDecomposition;
		rst.useNetOrdering = opts.useNetOrdering;
		rst.timeLimit = std::chrono::minutes(30);
		rst.emitSVG = opts.emitSVG;

		/// run actual routing
		rst.rrRoute();

		/// write the result
		writeToPath(opts.outputFile, problem);
		return 0;
	}
	catch (std::exception& ex) {
		printf("Error: %s\n", ex.what());
		return 1;
	}
}
