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
#include "PeriodicRunner.hpp"
#include "progress.hpp"
#include "colormap.hpp"

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


int RoutingInst::edgeWeight(int id) const
{
	size_t index = id;

	if(index < edgeInfos.size()) {
		return edgeInfos[index].weight;
	}
	else {
		return 0; // initial value
	}
	
}

int RoutingInst::edgeWeight(const Edge &e) const
{
	return edgeWeight(edgeID(e));
}

int RoutingInst::totalEdgeWeight(const Net &n) const
{
	int result = 0;

	for(const auto &route : n.nroute) {
		for(int edgeID : route.edges) {
			result += edgeWeight(edgeID);
		}
	}

	return result;
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



bool RoutingInst::hasViolation(const Net &n) const
{
	for(const auto &route : n.nroute) {
		for(int id : route.edges) {
			if(edgeUtils[id] > edgeCaps[id]) {
				return true;
			}
		}
	}

	return false;
}

void RoutingInst::aStarRouteSeg(Segment& s)
{
	unordered_set<Point> open;
	unordered_set<Point> closed;
	typedef std::pair<int, Point> CostPoint;
	
	auto costComp = [&](const CostPoint &p1, const CostPoint &p2) {
		return p1.first > p2.first;
	};
	
	priority_queue<CostPoint, vector<CostPoint>, decltype(costComp)> open_score(costComp);
	unordered_map<Point, Point> prev;

	Point p0;

	assert(s.edges.empty());

	// only the start node is initially open
	open.emplace(s.p1);
	open_score.emplace(s.p2.l1dist(s.p1), s.p1);

	// stop when end node is reach or when all nodes are explored
	while (closed.count(s.p2) == 0 && !open.empty()) {
		// move top canidate to 'closed' and evaluate neighbors
		p0 = open_score.top().second;
		open_score.pop();
		open.erase(p0);
		closed.emplace(p0);

		// add valid neighbors
		for(unsigned int neighborCase = 0; neighborCase < 4; ++neighborCase) {
			Point p = p0;
			if(!neighbor(p, neighborCase)) continue;

			// skip previously/currently examined
			if (closed.count(p) > 0 || open.count(p) > 0) {
				continue;
			}

			// queue valid neighbors for future examination
			open.emplace(p);
			open_score.emplace(s.p2.l1dist(p) + penalty*edgeUtil(p, p0) / (edgeCap(p, p0)+1), p);
			prev[p] = p0;
		}
	}

	// Walk backwards to create route
	for (Point p = s.p2; p != s.p1; p = prev[p]) {
		s.edges.emplace_back(edgeID(p, prev[p]));
	}
	
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
		n.nroute.emplace_back(it[0], it[1]);
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
			if (placed.count(edge) > 0) {
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
			if (ripped.count(edge) > 0) {
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


void RoutingInst::violationSvg(const std::string& fileName)
{
	ofstream svg(fileName);
	Edge e;

	svg << "<svg xmlns=\"http://www.w3.org/2000/svg\"";
	svg << "	xmlns:xlink=\"http://www.w3.org/1999/xlink\"\n";
	svg << "\twidth=\"" << gx*3 << "\" height=\"" << gy*3 << "\"";
	svg << ">\n";

	int maxOverflow = 0;
	for(size_t i = 0; i < edgeCaps.size(); ++i) {
		maxOverflow = max(maxOverflow, getElementOrDefault(edgeUtils, i, 0) - edgeCaps[i]);
	}
	
	for (unsigned int i = 0; i < edgeUtils.size(); i++) {
		e = edge(i);
		int overflow = edgeUtil(e.p1, e.p2) - edgeCap(e.p1, e.p2);
		if (overflow > 0) {
			svg << "\t<path stroke-width=\"3\" d=\"";
			svg << " M" << e.p1.x*3 << "," << e.p1.y*3;
			svg << " L" << e.p2.x*3 << "," << e.p2.y*3;
			svg << "\" style=\"stroke:" 
			    << getElementOrDefault(matplotlibHotColormap, overflow * 255 / maxOverflow, RGB{255,255,255})
			    << "; fill:none;\"/>\n";
		}
	
	}

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
	if (useNetOrdering)
		reorderNets();
	else
		printf("Not using ordering\n");

	int startTime = time(0);

	/// Print every 200 milliseconds
	PeriodicRunner<chrono::milliseconds> printer(chrono::milliseconds(200));
	ProgressBar pbar(cout);
	pbar.max = nets.size();

	int netsRouted = 0;

	
	auto printFunc = [&]()
	{
		pbar.value = netsRouted;
		pbar
			.draw()
			.writeln(setw(32), "Nets routed: ", netsRouted, "/", nets.size())
			.writeln(setw(32), "Elapsed time: ", time(0) - startTime, " seconds")
			.writeln(setw(32), "Overflow penalty: ", penalty);
	};

	// find an initial solution
	for (auto &n : nets) {
		routeNet(n);
		placeNet(n);
		++netsRouted;
		printer.runPeriodically(printFunc);
	}
	printFunc();

	logViolationSvg();
}

void RoutingInst::logViolationSvg()
{
	auto t = std::time(nullptr);
	auto tm = *std::localtime(&t);
	
    array<char, 256> timeString;

    strftime(timeString.data(), timeString.size(), "%Y%m%dT%H%M%S%z", &tm);
    std::stringstream ss;
    ss << "violations_" << timeString.data() << ".svg";
    auto filename = ss.str();
	
	violationSvg(filename);
	
	*htmlLog << "<div class=\"background\"><img src=\"" << filename << "\"></div>\n" << std::flush;
	std::cout << "violations logged to [" << filename << "]\n";
}
RoutingInst::RoutingInst()
: htmlLog(new std::ofstream("log.html"))
{
	*htmlLog << 
		"<!doctype html><html><head>\n"
		"<style type=\"text/css\">\n"
		".background { background-color: #000; }\n"
		"</style>\n"
		"<script src=\"http://code.jquery.com/jquery-1.11.0.min.js\"></script>"
		"<title>Log</title>\n";
	*htmlLog << "<script>\n";
	std::ifstream js("log.js");
	std::string line;
	while(getline(js, line)) {
		*htmlLog << line << "\n";
	}
	*htmlLog << "</script></head></body>\n";
}

RoutingInst::~RoutingInst()
{
	if(htmlLog.unique())
		*htmlLog << "</body></html>";
}
void RoutingInst::rrRoute()
{
	using std::chrono::system_clock;
	
	const auto procedureStartTime = system_clock::now();
	
	// get initial solution
	cout << "[1/2] Creating initial solution...\n";
	solveRouting();
	
	// rrr
	if(!useNetOrdering && !useNetDecomposition) return;
	cout << "[2/2] Rip up and reroute...\n";
	
	
	int deltaPenalty = -1;
	int deltaViolation = 0;
	int lastViolation = 0;
	const time_t startTime = time(nullptr);
	
	for(int iter = 0; iter < 15; ++iter) {
		if(system_clock::now() >= procedureStartTime + timeLimit) {
			cout << "Terminating due to expiration of time limit. Total time taken: " 
				<< chrono::duration_cast<chrono::seconds>(system_clock::now() - procedureStartTime).count()
				<< " seconds.\n";
			break;
		}
		cout << "--> Iteration " << iter << "\n";

		updateEdgeWeights();

		int violations = 0;
		for(auto &n : nets) if(hasViolation(n)) violations++;
		if(iter == 0) lastViolation = violations;
		deltaViolation = -lastViolation + violations;


		if(deltaViolation > 0) {
			deltaPenalty = -deltaPenalty;
		}
		penalty += deltaPenalty;
		

		lastViolation = violations;

		sort(nets.begin(), nets.end(), [&](const Net &n1, const Net &n2) {
			return totalEdgeWeight(n1) > totalEdgeWeight(n2);
		});

		ProgressBar pbar(cout);
		pbar.max = nets.size();
		PeriodicRunner<chrono::milliseconds> printer(chrono::milliseconds(200));
		int netsConsidered = 0;
		int netsRerouted = 0;

		auto printFunc = [&]()
		{
			pbar.value = netsConsidered;
			
			auto elap = system_clock::now() - procedureStartTime;
			int minutes = chrono::duration_cast<chrono::minutes>(elap).count();
			int seconds = int(chrono::duration_cast<chrono::seconds>(elap).count()) % 60;
			pbar
				.draw()
				.writeln(setw(32), "Nets considered: ", netsConsidered, "/", nets.size())
				.writeln(setw(32), "Nets rerouted: ", netsRerouted)
				.writeln(setw(32), "Phase time elapsed: ", time(nullptr) - startTime, " seconds")
				.writeln(setw(32), "Total time elapsed: ", 
				         minutes, ":", setw(2), setfill('0'), seconds, setfill(' '))
				.writeln(setw(32), "Overflow penalty: ", penalty)
				.writeln(setw(32), "Violations: ", lastViolation, " (delta ", deltaViolation, ")");
		};


		for(auto &n : nets) {
			if(hasViolation(n)) {
				ripNet(n);
				assert(n.nroute.empty());

				decomposeNet(n);
				for (auto &s : n.nroute) {
					aStarRouteSeg(s);
				}

				placeNet(n);

				++netsRerouted;
			}
			++netsConsidered;

			printer.runPeriodically(printFunc);
		}
		printFunc();
		logViolationSvg();
	}
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
