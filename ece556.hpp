// ECE556 - Copyright 2014 University of Wisconsin-Madison.  All Rights Reserved.

#pragma once

#include <cstdlib> // for integer abs
#include <vector>
#include <iosfwd>
#include "edgeid.hpp"
#include "util.hpp"
#include <fstream>
#include <memory>

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

namespace std {
	template <>
	struct hash<Point> {
		std::size_t operator()(const Point& p) const
		{
			return static_cast<size_t>(p.x) ^ (static_cast<size_t>(p.y) << (sizeof(size_t) / 2 * 8));
		}
	};
}

/// A segment consisting of two points and edges
/// This is not, strictly speaking, a line segment, but is actually a linear spline (a collection of
/// connected line segments).
struct Segment {
	Point p1 ; ///< start point of a segment
	Point p2 ; ///< end point of a segment

	std::vector<int> edges; ///< Edges representing the segment

	Segment() = default;

	Segment(const Point& cp1, const Point& cp2) :
		p1(cp1),
		p2(cp2),
		edges()
	{ }

	Segment(const Segment&) = default;

	Segment(Segment&& o) :
		p1(o.p1),
		p2(o.p2),
		edges(move(o.edges))
	{ }

	Segment& operator=(const Segment&) = default;

	Segment& operator=(Segment&& o)
	{
		p1 = o.p1;
		p2 = o.p2;
		edges = std::move(o.edges);
		return *this;
	}
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

	int pinTourManhattan() const
	{
		int r = 0;

		for(auto it = pins.begin() + 1, end = pins.end(); it != end; ++it) {
			r += it[-1].l1dist(it[0]);
		}

		return r;
	}
};

/// Represents a routing instance
struct RoutingInst {
private:
	void decomposeNetSimple(Net &n);
	void decomposeNetMST(Net &n);

	struct EdgeInfo
	{
		int overflowCount; // k_e^k
		int weight; // w_e^k
		
		EdgeInfo()
		: overflowCount(0)
		, weight(0)
		{ }
	};
	std::vector<EdgeInfo> edgeInfos;
	std::shared_ptr<std::ofstream> htmlLog;

	void updateEdgeWeights();
	int edgeWeight(const Edge &e) const;
	int edgeWeight(int id) const;
	int totalEdgeWeight(const Net &n) const;
	bool hasViolation(const Net &n) const;

	int penalty = 15;
public:
	RoutingInst();
	~RoutingInst();

	bool useNetDecomposition = true;
	bool useNetOrdering = true;

	int gx; ///< x dimension of the global routing grid
	int gy; ///< y dimension of the global routing grid

	int cap;

	std::vector<Net> nets;

	int numEdges; ///< number of edges of the grid
	std::vector<int> edgeCaps; ///< array of the actual edge capacities after considering for blockage
	std::vector<int> edgeUtils; ///< array of edge utilizations
	

	bool neighbor(Point &p, unsigned int caseNumber);

	/// Use A* search to route a segment with the overflow 
	/// penalty from the member variable `penalty`.
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
	void logViolationSvg();
	void rrRoute();

	int edgeID(const Point &p1, const Point &p2) const
	{
		return ::edgeID(gx, gy, p1.x, p1.y, p2.x, p2.y);
	}

	int edgeID(const Edge &e) const
	{
		return edgeID(e.p1, e.p2);
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

