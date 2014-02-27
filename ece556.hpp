// ECE556 - Copyright 2014 University of Wisconsin-Madison.  All Rights Reserved.

#pragma once

#include <vector>

/**
 * A structure to represent a 2D Point.
 */
struct Point {

	int x; ///< x coordinate ( >=0 in the routing grid)
	int y; ///< y coordinate ( >=0 in the routing grid)
};


/**
 * A structure to represent a segment
 */
struct Segment {

	Point p1 ; ///< start point of a segment
	Point p2 ; ///< end point of a segment

	std::vector<int> edges; ///< Edges representing the segment
};

typedef std::vector<Segment> Route;


/**
 * A structure to represent nets
 */
struct Net {

	int id; ///< ID of the net

	std::vector<Point> pins; ///< pins (or terminals) of the net
	Route nroute; ///< stored route for the net.
};

/**
 * A structure to represent the routing instance
 */
struct RoutingInst
{
	int gx; ///< x dimension of the global routing grid
	int gy; ///< y dimension of the global routing grid

	int cap;

	std::vector<Net> nets;

	int numEdges; ///< number of edges of the grid
	std::vector<int> edgeCaps; ///< array of the actual edge capacities after considering for blockage
	std::vector<int> edgeUtils; ///< array of edge utilizations

};


/* int readBenchmark(const char *fileName, routingInst *rst)
 * Read in the benchmark file and initialize the routing instance.
 * This function needs to populate all fields of the routingInst structure.
 * input1: fileName: Name of the benchmark input file
 * input2: pointer to the routing instance
 * output: 1 if successful
 */
int readBenchmark(const char *fileName, routingInst *rst);


/* int solveRouting(routingInst *rst)
 * This function creates a routing solution
 * input: pointer to the routing instance
 * output: 1 if successful, 0 otherwise (e.g. the data structures are not populated)
 */
int solveRouting(routingInst *rst);

/* int writeOutput(const char *outRouteFile, routingInst *rst)
 * Write the routing solution obtained from solveRouting().
 * Refer to the project link for the required output format.

 * Finally, make sure your generated output file passes the evaluation script to make sure
 * it is in the correct format and the nets have been correctly routed. The script also reports
 * the total wirelength and overflow of your routing solution.

 * input1: name of the output file
 * input2: pointer to the routing instance
 * output: 1 if successful, 0 otherwise
 */
int writeOutput(const char *outRouteFile, routingInst *rst);
