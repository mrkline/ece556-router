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


/**
 * \brief Read in the benchmark file and initialize the routing instance.
 * \param fileName Name of the benchmark input file
 * \param rst The to the routing instance to populate
 *
 * This function needs to populate all fields of the routingInst structure.
 */
void readBenchmark(const char *fileName, RoutingInst& rst);


/**
 * \brief reates a routing solution
 * \param rst The routing instance
 */
void solveRouting(RoutingInst& rst);

/**
 * \brief Write the routing solution obtained from solveRouting().
 * \param outRouteFile name of the output file
 * \param rst The routing instance
 *
 * Refer to the project link for the required output format.
 * Finally, make sure your generated output file passes the evaluation script to make sure
 * it is in the correct format and the nets have been correctly routed. The script also reports
 * the total wirelength and overflow of your routing solution.
 */
void writeOutput(const char *outRouteFile, RoutingInst& rst);
