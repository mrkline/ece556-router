// ECE556 - Copyright 2014 University of Wisconsin-Madison.  All Rights Reserved.

#include <cassert>
#include <fstream>
#include <iostream>
#include <sstream>
#include <queue>
#include <unordered_map>
#include <unordered_set>

#include "ece556.hpp"
#include "reader.hpp"
#include "util.hpp"

void readBenchmark(const char *fileName, RoutingInst& rst)
{
	std::ifstream in(fileName);
	if(!in) {
		throw std::runtime_error("I/O Error");
	}
	rst = readRoutingInst(in);
}

std::vector<Point> RoutingInst::findNeighbors(const Point& p0)
{
	std::vector<Point> neighbors;
	Point p;

	// find neighbors
	p = p0;
	if (--p.x >= 0) {
		neighbors.push_back(p);
	}
	p = p0;
	if (++p.x < gx) {
		neighbors.push_back(p);
	}
	p = p0;
	if (--p.y >= 0) {
		neighbors.push_back(p);
	}
	p = p0;
	if (++p.y < gy) {
		neighbors.push_back(p);
	}

	return neighbors;
}

// Use A* search to route a segment
void RoutingInst::aStarSegRoute(Segment& s)
{
	std::unordered_set<Point, PointHash> open, closed;
	std::priority_queue<Point, std::vector<Point>, GoalComp>
		open_score(GoalComp{s.p2});
	std::unordered_map<Point, Point, PointHash> prev;
	std::vector<Point> neighbors;
	Point p0, p;

	assert(s.edges.empty());

	// only the start node is initially open
	open.insert(s.p1);
	open_score.push(s.p1);

	// stop only when the 'end' node is reached
	// TODO: need a bail-out if it takes too long!!
	while (!closed.count(s.p2)) {
		// move top canidate to 'closed' and evaluate neighbors
		p0 = open_score.top();
		open_score.pop();
		open.erase(p0);
		closed.insert(p0);

		// add valid neighbors
		neighbors = findNeighbors(p0);
		for (const auto &p : neighbors) {
			// skip previously/currently examined
			if (closed.count(p) || open.count(p)) {
				continue;
			}

			// skip blocked edges
			if (edgeUtil(p, p0) >= edgeCap(p, p0)) {
				continue;
			}

			// queue valid neighbors for future examination
			open.insert(p);
			open_score.push(p);
			prev[p] = p0;
		}
	}

	// Walk backwards to create route
	for (p = s.p2; p != s.p1; p = prev[p]) {
		s.edges.push_back(edgeID(p, prev[p]));
	}
}

// decompose pins for an unrouted net into unrouted segments
void RoutingInst::decomposeNet(Net& n)
{
	unsigned int i;
	Segment s;

	assert(n.nroute.empty());

	for (i = 1; i < n.pins.size(); i++) {
		s.p1 = n.pins[i - 1];
		s.p2 = n.pins[i];
		n.nroute.push_back(s);
	}
}

// route an unrouted net
void RoutingInst::routeNet(Net& n)
{
	assert(n.nroute.empty());

	decomposeNet(n);
	for (auto &s : n.nroute) {
		// TODO: deal with conflicts between unrouted segments??
		aStarSegRoute(s);
	}
}

void RoutingInst::placeRoute(const Net& n)
{
	for (const auto s : n.nroute) {
		for (const auto i : s.edges) {
			getElementResizingIfNecessary(edgeUtils, i, 0)++;
		}
	}
}

// rip up the route from an old net and return it
Route RoutingInst::ripNet(Net& n)
{
	Route old;
	for (const auto &s : n.nroute) {
		for (const auto i : s.edges) {
			getElementResizingIfNecessary(edgeUtils, i, 0)--;
		}
	}
	n.nroute.swap(old);
	return old;
}

int RoutingInst::countViolations()
{
	int v = 0;
	for (unsigned int i = 0; i < edgeUtils.size(); i++) {
		if (edgeUtils[i] > edgeCaps[i]) {
			v++;
		}
	}

	return v;
}

bool RoutingInst::routeValid(Route& r, bool isplaced)
{
	(void)r;
	(void)isplaced;
	return false;
}

void RoutingInst::toSvg(const std::string& fileName)
{
	std::ofstream svg(fileName);

	svg << "<svg xmlns=\"http://www.w3.org/2000/svg\"";
	svg << "	xmlns:xlink=\"http://www.w3.org/1999/xlink\">\n";

	Edge e;
	for (const auto n : nets) {
		for (const auto s : n.nroute) {
			svg << "\t<path d=\"";
			for (const auto i : s.edges) {
				e = edge(i);
				svg << "M" << e.p1.x * 2 << "," << e.p1.y * 2;
				svg << " L" << e.p2.x * 2 << "," << e.p2.y * 2;
			}
			svg << "\" style=\"stroke:#FF0000; fill:none;\"/>\n";
		}
	}
	svg << "</svg>";
	svg.close();
	
}

void RoutingInst::solveRouting()
{
	std::stringstream ss;

	// find an initial solution
	for (auto &n : nets) {
		routeNet(n);
		placeRoute(n);

		ss.str("net_");
		ss << n.id << ".svg";
		toSvg(ss.str());
	}

	// find violators
	/*for (const auto &n : rst.nets)
		if (hasViolations(n.route, rst))
			v_nets.push(n);

	// reorder nets (TODO)

	// reroute until all our problems are solved!
	while (!v_nets.empty()) {
		const auto &n = v_nets.top();
		v_nets.pop();

		if (!hasViolators(n, true, rst))
			continue;

		// rip up and reroute
		ripRoute(n.nroute, rst);
		findRoute(n.pin, &rnew, rst);
		if (!hasViolators(rnew, false, rst))
			n.nroute = rnew;
		else
			v_nets.push(n);
		placeRoute(n.nroute, rst);
	}*/
}

void RoutingInst::writeOutput(const char *outRouteFile)
{
	(void)outRouteFile;
}


std::ostream &operator <<(std::ostream &os, const Point &p)
{
	return os << "Point{" << p.x << ", " << p.y << "}";
}

std::ostream &operator <<(std::ostream &os, const Edge &e)
{
	return os << "Edge{" << e.p1 << ", " << e.p2 << "}";
}
