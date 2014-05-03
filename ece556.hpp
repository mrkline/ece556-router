// ECE556 - Copyright 2014 University of Wisconsin-Madison.  All Rights Reserved.

#pragma once

#include <cstdlib> // for integer abs
#include <vector>
#include <iosfwd>
#include "edgeid.hpp"
#include "util.hpp"
#include <fstream>
#include <memory>
#include <chrono>

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

/// A path consisting of a start, an end, and edges between the two
struct Path {
	Point p1 ; ///< start point of a segment
	Point p2 ; ///< end point of a segment

	std::vector<int> edges; ///< Edges representing the segment

	Path() = default;

	Path(const Point& cp1, const Point& cp2) :
		p1(cp1),
		p2(cp2),
		edges()
	{ }

	Path(const Path&) = default;

	Path(Path&& o) :
		p1(o.p1),
		p2(o.p2),
		edges(move(o.edges))
	{ }

	Path& operator=(const Path&) = default;

	Path& operator=(Path&& o)
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

/// A route is a series of paths
typedef std::vector<Path> Route;


/// A structure to represent nets
struct Net {

	int id; ///< ID of the net

	int totalEdgeWeight = 0; // for sorting
	int netSpan = 0; // for sorting

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

std::ostream &operator <<(std::ostream &, const Point &);
std::ostream &operator <<(std::ostream &, const Edge &);

