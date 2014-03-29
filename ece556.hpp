// ECE556 - Copyright 2014 University of Wisconsin-Madison.  All Rights Reserved.

#pragma once

#include <cstdlib> // for integer abs
#include <vector>
#include <iosfwd>
#include "edgeid.hpp"
#include "util.hpp"

/// Represents a 2D Point
struct Point {

	int x; ///< x coordinate ( >=0 in the routing grid)
	int y; ///< y coordinate ( >=0 in the routing grid)

	int l1dist(const Point& p) const
	{
		return abs(x - p.x) + abs(y - p.y);
	}

	bool operator ==(const Point &that) const
	{
		return x == that.x && y == that.y;
	}
	bool operator !=(const Point &that) const
	{
		return x != that.x || y != that.y;
	}
};

struct PointHash {
	std::size_t operator()(const Point& p) const
	{
		// TODO: check if this is true on tux machines
		static_assert(sizeof(std::size_t) == 8, "sizeof size_t should be 8 bytes");
		return static_cast<std::size_t>(p.x) | (static_cast<std::size_t>(p.y) << 32);
	}
};


struct GoalComp {
	Point goal;

	bool operator()(const Point& p1, const Point& p2) const
	{
		return goal.l1dist(p1) > goal.l1dist(p2);
	}
};

/// A segment consisting of two points and edges
/// This is not, strictly speaking, a line segment, but is actually a linear spline (a collection of
/// connected line segments).
struct Segment {
	Point p1 ; ///< start point of a segment
	Point p2 ; ///< end point of a segment
	
	std::vector<int> edges; ///< Edges representing the segment
};

/// This is a real line segment. In this router, they are always 1 unit long.
struct Edge {
	Point p1, p2;

	static Edge horizontal(Point p1) 
	{
		return Edge{p1, {p1.x+1, p1.y}};
	}

	static Edge vertical(Point p1)
	{
		return Edge{p1, {p1.x, p1.y+1}};
	}


	bool operator ==(const Edge &that) const 
	{
		return p1 == that.p1 && p2 == that.p2;
	}
};

/// A route is a series of segments
typedef std::vector<Segment> Route;


/// A structure to represent nets
struct Net {

	int id; ///< ID of the net

	std::vector<Point> pins; ///< pins (or terminals) of the net
	Route nroute; ///< stored route for the net.
};

/// Represents a routing instance
struct RoutingInst {
	int gx; ///< x dimension of the global routing grid
	int gy; ///< y dimension of the global routing grid

	int cap;
	int aggression;
	int startHi = 10;

	std::vector<Net> nets;

	int numEdges; ///< number of edges of the grid
	std::vector<int> edgeCaps; ///< array of the actual edge capacities after considering for blockage
	std::vector<int> edgeUtils; ///< array of edge utilizations


	std::vector<Point> findNeighbors(const Point& p0);
	
	bool neighbor(Point &p, unsigned int caseNumber);
	
	bool _aStarRouteSeg(Segment& s, int aggressiveness);
	
	void aStarRouteSeg(Segment& s);
	void decomposeNet(Net& n);
	void routeNet(Net& n);
	void placeNet(const Net& n);
	Route ripNet(Net& n);
	int countViolations();
	bool routeValid(Route& r, bool isplaced);
	void reorderNets();
	void solveRouting();
	void writeOutput(const char *outRouteFile);
	void violationSvg(const std::string& fileName);
	void toSvg(const std::string& fileName);
	

	int edgeID(const Point &p1, const Point &p2) const
	{
		return ::edgeID(gx, gy, p1.x, p1.y, p2.x, p2.y);
	}

	Edge edge(int edgeID) const
	{
		return ::edge(gx, gy, edgeID);
	}

	void setEdgeUtil(const Point &p1, const Point &p2, int util)
	{
		int id = edgeID(p1, p2);
		getElementResizingIfNecessary(edgeUtils, id, 0) = util;
	}

	int edgeUtil(const Point &p1, const Point &p2) const
	{
		int id = edgeID(p1, p2);
		return getElementOrDefault(edgeUtils, id, 0);
	}

	void setEdgeCap(const Point &p1, const Point &p2, int capacity)
	{
		int id = edgeID(p1, p2);
		getElementResizingIfNecessary(edgeCaps, id, cap) = capacity;
	}

	int edgeCap(const Point &p1, const Point &p2) const
	{
		return getElementOrDefault(edgeCaps, edgeID(p1, p2), cap);
	}
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


std::ostream &operator <<(std::ostream &, const Point &);
std::ostream &operator <<(std::ostream &, const Edge &);

