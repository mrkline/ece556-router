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

// Use A* search to route a segment with a maximum of aggressiveness violation on each edge
bool RoutingInst::_aStarRouteSeg(Segment& s, int aggressiveness)
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

	// stop when end node is reach or when all nodes are explored
	while (!closed.count(s.p2) && !open.empty()) {
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
			if (edgeUtil(p, p0) >= edgeCap(p, p0) + aggressiveness) {
				continue;
			}

			// queue valid neighbors for future examination
			open.insert(p);
			open_score.push(p);
			prev[p] = p0;
		}
	}

	// couldn't route without violation... BAIL!!
	if (!closed.count(s.p2)) {
		return false;
	}

	// Walk backwards to create route
	for (p = s.p2; p != s.p1; p = prev[p]) {
		s.edges.push_back(edgeID(p, prev[p]));
	}

	return true;
}

void RoutingInst::aStarRouteSeg(Segment& s)
{
	int a = 0;
	while (!_aStarRouteSeg(s, a++)) {
		std::cout << "GETTING MORE AGGRESSIVE!!\n";
	}
	return;
}

// decompose pins into a MST based on L1 distance
void RoutingInst::decomposeNet(Net& n)
{
	Segment s;
	std::unordered_map<Point, Point, PointHash> adj;
	std::unordered_map<Point, int, PointHash> dist;
	std::unordered_set<Point, PointHash> q;

	Point p0;
	int min, t;

	assert(n.nroute.empty());

	// place the first node in the tree
	p0 = n.pins[0];
	for (const auto &p : n.pins) {
		q.insert(p);
		adj[p] = p0;
		dist[p] = p0.l1dist(p);
	}
	q.erase(p0);

	while (!q.empty()) {
		// find the non-tree node closest to a tree node
		p0 = *q.begin(); min = dist[p0];
		for (const auto &p : q) {
			if (dist[p] <= min) {
				p0 = p;
				min = dist[p];
			}
		}

		// attach the non-tree node to its closest tree neighbor
		q.erase(p0);
		s.p1 = p0;
		s.p2 = adj[p0];
		n.nroute.push_back(s);

		// the tree has a new node; update the map of closest nodes
		for (const auto &p : q) {
			t = p0.l1dist(p);
			if (t < dist[p]) {
				adj[p] = p0;
				dist[p] = t;
			}
		}
	}
}

// route an unrouted net
void RoutingInst::routeNet(Net& n)
{
	assert(n.nroute.empty());

	decomposeNet(n);
	for (auto &s : n.nroute) {
		aStarRouteSeg(s);
	}
	// TODO: should inter-segment conflict be handled here, or elsewhere?
}

void RoutingInst::placeNet(const Net& n)
{
	std::unordered_set<int> placed;

	for (const auto s : n.nroute) {
		for (const auto edge : s.edges) {
			if (placed.count(edge)) {
				continue;
			}
			getElementResizingIfNecessary(edgeUtils, edge, 0)++;
			placed.insert(edge);
		}
	}
}

// rip up the route from an old net and return it
Route RoutingInst::ripNet(Net& n)
{
	std::unordered_set<int> ripped;
	Route old;

	for (const auto &s : n.nroute) {
		for (const auto edge : s.edges) {
			if (ripped.count(edge)) {
				continue;
			}
			getElementResizingIfNecessary(edgeUtils, edge, 0)--;
			ripped.insert(edge);
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

void RoutingInst::violationSvg(const std::string& fileName)
{
	std::ofstream svg(fileName);
	Edge e;

	svg << "<svg xmlns=\"http://www.w3.org/2000/svg\"";
	svg << "	xmlns:xlink=\"http://www.w3.org/1999/xlink\">\n";

	svg << "\t<path d=\"";
	for (unsigned int i = 0; i < edgeUtils.size(); i++) {
		e = edge(i);
		if (edgeUtil(e.p1, e.p2) > edgeCap(e.p1, e.p2)) {
			svg << " M" << e.p1.x << "," << e.p1.y;
			svg << " L" << e.p2.x << "," << e.p2.y;
		}
	}
	svg << "\" style=\"stroke:#FF0000; fill:none;\"/>\n";
	svg << "</svg>";
	svg.close();
	
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
				svg << " M" << e.p1.x << "," << e.p1.y;
				svg << " L" << e.p2.x << "," << e.p2.y;
			}
			svg << "\" style=\"stroke:#FF0000; fill:none;\"/>\n";
		}
	}
	svg << "</svg>";
	svg.close();
	
}

void RoutingInst::reorderNets()
{


}

void RoutingInst::solveRouting()
{
	std::stringstream ss;

	reorderNets();
	int i = 0;

	// find an initial solution
	for (auto &n : nets) {
		//if (n.id != 12660) continue;
		std::cout << n.id << "\n";
		routeNet(n);
		placeNet(n);

		//ss.str("net_");
		//ss << n.id << ".svg";
		//toSvg(ss.str());
		if (i++ > 20000) break;;
	}

	/*for (auto &n : nets) {
		placeRoute(n);
	}*/
	violationSvg("violations.svg");

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
