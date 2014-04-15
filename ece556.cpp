// ECE556 - Copyright 2014 University of Wisconsin-Madison.  All Rights Reserved.

#include <algorithm>
#include <cassert>
#include <cstring>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <queue>
#include <sstream>
#include <unordered_map>
#include <unordered_set>


#include "ece556.hpp"
#include "reader.hpp"
#include "writer.hpp"
#include "util.hpp"

using namespace std;

void readBenchmark(const char *fileName, RoutingInst& rst)
{
	ifstream in(fileName);
	if(!in) {
		throw runtime_error("I/O Error");
	}
	rst = readRoutingInst(in);
}

void RoutingInst::updateEdgeWeights()
{
	edgeInfos.resize(edgeCaps.size());

	for(size_t i = 0; i < edgeInfos.size(); ++i) {
		auto &ei = edgeInfos[i];
		const int overflow = edgeUtils[i] - edgeCaps[i];

		if(overflow > 0) {
			ei.weight = overflow * ei.overflowCount;
			ei.overflowCount++;
		}
		else {
			ei.weight = 0;
		}
	}
}

int RoutingInst::edgeWeight(const Edge &e) const
{
	size_t index = edgeID(e);

	if(index < edgeInfos.size()) {
		return edgeInfos[index].weight;
	}
	else {
		return 0; // initial value
	}
}


bool RoutingInst::neighbor(Point &p, unsigned int caseNumber)
{
	switch(caseNumber)
	{
		case 0:
			return --p.x >= 0;
		case 1:
			return ++p.x < gx;
		case 2:
			return --p.y >= 0;
		case 3:
			return ++p.y < gy;
		default:
			return false;
	}
}


// Use A* search to route a segment with a maximum of aggressiveness violation on each edge
bool RoutingInst::_aStarRouteSeg(Segment& s, int aggressiveness)
{
	unordered_set<Point> open, closed;
	priority_queue<Point, vector<Point>, GoalComp>
		open_score(GoalComp{s.p2});
	unordered_map<Point, Point> prev;
	vector<Point> neighbors;

	Point p0, p;

	assert(s.edges.empty());

	// only the start node is initially open
	open.emplace(s.p1);
	open_score.emplace(s.p1);

	// stop when end node is reach or when all nodes are explored
	while (!closed.count(s.p2) && !open.empty()) {
		// move top canidate to 'closed' and evaluate neighbors
		p0 = open_score.top();
		open_score.pop();
		open.erase(p0);
		closed.emplace(p0);

		// add valid neighbors
		for(unsigned int neighborCase = 0; neighborCase < 4; ++neighborCase) {
			Point p = p0;
			if(!neighbor(p, neighborCase)) continue;

			// skip previously/currently examined
			if (closed.count(p) || open.count(p)) {
				continue;
			}

			// skip blocked edges
			if (edgeUtil(p, p0) >= edgeCap(p, p0) + aggressiveness) {
				continue;
			}

			// queue valid neighbors for future examination
			open.emplace(p);
			open_score.emplace(p);
			prev[p] = p0;
		}
	}

	// couldn't route without violation... BAIL!!
	if (!closed.count(s.p2)) {
		return false;
	}

	// Walk backwards to create route
	for (p = s.p2; p != s.p1; p = prev[p]) {
		s.edges.emplace_back(edgeID(p, prev[p]));
	}

	return true;
}

void RoutingInst::aStarRouteSeg(Segment& s)
{
	int lo=0, hi=startHi;
	int v=aggression;
	int routed = -1;
	int maxSuccessfulBisections = 1;
	int successfulBisections = 0;

	Segment soln;
	while(routed < 0) {
		while(hi - lo > 1) {
			s.edges.clear();
			if(_aStarRouteSeg(s, v)) {
				soln = s;
				hi = v;
				routed = v;
				successfulBisections++;
			}
			else {
				lo = v;
			}
			v = (lo + hi) / 2;

			if(successfulBisections >= maxSuccessfulBisections) {
				v = hi;
				goto exit;
			}
		}

		hi += 10;
		if(hi > startHi) {
			startHi = hi;
		}
	}
exit:
	s = soln;

	aggression = (aggression + routed) / 2;
	return;
}

void RoutingInst::decomposeNetMST(Net &n)
{
	Segment s;

	unordered_map<Point, Point> adj;
	unordered_map<Point, int> dist;
	unordered_set<Point> q;


	adj.clear();
	dist.clear();
	q.clear();

	Point p0;
	int min, t;

	assert(n.nroute.empty());

	// place the first node in the tree
	p0 = n.pins[0];
	for (const auto &p : n.pins) {
		q.emplace(p);
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
		n.nroute.emplace_back(s);

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

void RoutingInst::decomposeNetSimple(Net &n)
{
	if(n.pins.empty()) return; // (assumed nonempty by loop condition)
	
	for(auto it = n.pins.begin(); it + 1 != n.pins.end(); ++it) {
		n.nroute.emplace_back<Segment>({it[0], it[1], {}});
	}
}


// decompose pins into a MST based on L1 distance
void RoutingInst::decomposeNet(Net& n)
{
	if(useNetDecomposition) {
		decomposeNetMST(n);
	}
	else {
		decomposeNetSimple(n);
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
	unordered_set<int> placed;

	for (const auto s : n.nroute) {
		for (const auto edge : s.edges) {
			if (placed.count(edge)) {
				continue;
			}
			getElementResizingIfNecessary(edgeUtils, edge, 0)++;
			placed.emplace(edge);
		}
	}
}

// rip up the route from an old net and return it
Route RoutingInst::ripNet(Net& n)
{
	unordered_set<int> ripped;
	Route old;

	for (const auto &s : n.nroute) {
		for (const auto edge : s.edges) {
			if (ripped.count(edge)) {
				continue;
			}
			getElementResizingIfNecessary(edgeUtils, edge, 0)--;
			ripped.emplace(edge);
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
	ofstream svg(fileName);
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
	ofstream svg(fileName);

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

	sort(nets.begin(), nets.end(), [](const Net &n1, const Net &n2) {
		return n2.pins.size() > n1.pins.size();
// 		return n1.pinTourManhattan() > n2.pinTourManhattan();
	});
}

void RoutingInst::solveRouting()
{
	aggression = 0;
	stringstream ss;

	reorderNets();
	int i = 0;

	int barWidth = 60;
	int barDivisor = nets.size() / barWidth;
	int startTime = time(0);

	cout << "\n\n\n\n";
	// find an initial solution
	for (auto &n : nets) {
		//if (n.id != 12660) continue;
// 		cout << n.id << "\n";
		routeNet(n);
		placeNet(n);

// 		++i;
		if((i++ % 512) == 0)
		{
			int width = i / barDivisor;
			cout << "\033[3A\033[1G" << flush;
			cout << "\033[0K" << setw(4) << i * 100 / nets.size() << " [";
			for(int j = 0; j < width; ++j)
			{
				cout << "*";
			}
			for(int j = width; j < barWidth; ++j)
			{
				cout << "-";
			}

			cout << "]";

			cout << "\n\033[0KNets routed: " << i << "/" << nets.size();
			cout << "\n\033[0KElapsed time: " << time(0) - startTime << " seconds. "
				<< "Aggression level: " <<  aggression << ". Bisect max: " << startHi << ". TOF: " << tof << "\n";

		}

// 		if((i % 2048) == 0) {
// 			ss.str("net_");
// 			ss << n.id << ".svg";
// 			toSvg(ss.str());
// 		}
// 		if (i > 20000) break;;
	}

	/*for (auto &n : nets) {
		placeRoute(n);
	}*/
	violationSvg("violations.svg");

	// find violators
	/*for (const auto &n : rst.nets)
		if (hasViolations(n.route, rst))
			v_nets.emplace(n);

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
			v_nets.emplace(n);
		placeRoute(n.nroute, rst);
	}*/
}

namespace {

string strerrno()
{
	return strerror(errno);
}

} // end anonymous namespace

void RoutingInst::writeOutput(const char *outRouteFile)
{
	ofstream out(outRouteFile);
	if(!out) {
		throw runtime_error(string("opening ") + outRouteFile + ": " + strerrno());
	}
	write(out, *this);
	out.close(); // close explicitly to allow for printing a warning message if it fails

	if(!out) {
		cerr << "warning: error closing file: " << strerrno() << "\n";
	}
}


std::ostream &operator <<(std::ostream &os, const Point &p)
{
	return os << "Point{" << p.x << ", " << p.y << "}";
}

std::ostream &operator <<(std::ostream &os, const Edge &e)
{
	return os << "Edge{" << e.p1 << ", " << e.p2 << "}";
}
