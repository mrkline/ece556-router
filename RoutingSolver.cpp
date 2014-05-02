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
#include <csignal>
#include <thread>
#include <future>


#include "ece556.hpp"
#include "reader.hpp"
#include "writer.hpp"
#include "util.hpp"
#include "PeriodicRunner.hpp"
#include "progress.hpp"
#include "colormap.hpp"
#include "RoutingInst.hpp"
#include "IteratorUtils.hpp"

#include "RoutingSolver.hpp"

using namespace std;

void RoutingSolver::updateEdgeWeights()
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


int RoutingSolver::edgeWeight(int id) const
{
	size_t index = id;

	if(index < edgeInfos.size()) {
		return edgeInfos[index].weight;
	}
	else {
		return 0; // initial value
	}
	
}

int RoutingSolver::netSpan(const Net &n) const
{
	unordered_set<int> counted;
	int result = 0;

	for(const auto &route : n.nroute) {
		for(int edgeID : route.edges) {
			if (counted.count(edgeID) > 0) continue;

			counted.emplace(edgeID);
			result++;
		}
	}

	return result;
}

int RoutingSolver::edgeWeight(const Edge &e) const
{
	return edgeWeight(edgeID(e));
}

int RoutingSolver::totalEdgeWeight(const Net &n) const
{
	unordered_set<int> counted;
	int result = 0;

	for(const auto &route : n.nroute) {
		for(int edgeID : route.edges) {
			if (counted.count(edgeID) > 0) continue;

			counted.emplace(edgeID);
			result += edgeWeight(edgeID);
		}
	}

	return result;
}


bool RoutingSolver::neighbor(Point &p, unsigned int caseNumber)
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



bool RoutingSolver::hasViolation(const Net &n) const
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

void RoutingSolver::aStarRouteSeg(Path& s)
{
	unordered_set<Point> open;
	unordered_set<Point> closed;
	typedef std::pair<int, Point> CostPoint;
	
	auto costComp = [&](const CostPoint &p1, const CostPoint &p2) {
		return p1.first + s.p2.l1dist(p1.second) >
			p2.first + s.p2.l1dist(p2.second);
	};
	
	priority_queue<CostPoint, vector<CostPoint>, decltype(costComp)> open_score(costComp);
	unordered_map<Point, Point> prev;

	Point p0;
	int p0_cost;

	assert(s.edges.empty());

	// only the start node is initially open
	open.emplace(s.p1);
	open_score.emplace(0, s.p1);

	// stop when end node is reach or when all nodes are explored
	while (closed.count(s.p2) == 0 && !open.empty()) {
		// move top canidate to 'closed' and evaluate neighbors
		p0_cost = open_score.top().first;
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
			open_score.emplace(
				p0_cost + 1 +
				penalty*edgeUtil(p, p0) / (edgeCap(p, p0)+1),
				p);
			prev[p] = p0;
		}
	}

	// Walk backwards to create route
	for (Point p = s.p2; p != s.p1; p = prev[p]) {
		s.edges.emplace_back(edgeID(p, prev[p]));
	}
	
}

void decomposeNets(std::vector<Net>& nets, bool useNetDecomposition)
{
	auto partitionPoints = partitionCollection(begin(nets), end(nets),
	                                           max(2u, thread::hardware_concurrency()));


	/* Lambdas in lambdas. Does anyone else love lambdas? Yaaaay lambdas!
	 * But actually, what the hell are we doing here?
	 * 1. We just broke the list of nets into equally-ish sized bits
	 *    based on the number of cores we have to run threads on.
	 * 2. For each of those bits, run a seperate thread which decomposes the nets.
	 * 3. Wait for them to finish.
	 *
	 * Check out std::future and std::async for each of these operations
	 */
	vector<future<void>> futures;
	for (size_t i = 1; i < partitionPoints.size(); ++i) {
		futures.emplace_back(async(launch::async, [=] {
			for_each(partitionPoints[i - 1], partitionPoints[i], [=](Net& n) {
				decomposeNet(n, useNetDecomposition);
			});
		}));
	}

	for (auto& future : futures)
		future.wait();
}

void decomposeNetMST(Net &n)
{
	Path s;

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

void decomposeNetSimple(Net &n)
{
	if(n.pins.empty()) return; // (assumed nonempty by loop condition)
	
	for(auto it = n.pins.begin(); it + 1 != n.pins.end(); ++it) {
		n.nroute.emplace_back(it[0], it[1]);
	}
}


// decompose pins into a MST based on L1 distance
void decomposeNet(Net& n, bool useNetDecomposition)
{
	if(useNetDecomposition) {
		decomposeNetMST(n);
	}
	else {
		decomposeNetSimple(n);
	}
}

// route an unrouted net
void RoutingSolver::routeNet(Net& n)
{
	assert(n.nroute.empty());

	#pragma omp parallel for
	for (unsigned int i = 0; i < n.nroute.size(); i++) {
		aStarRouteSeg(n.nroute[i]);
	}
	// TODO: should inter-segment conflict be handled here, or elsewhere?
}

void RoutingSolver::placeNet(const Net& n)
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
Route RoutingSolver::ripNet(Net& n)
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

int RoutingSolver::countViolations()
{
	int v = 0;
	for (unsigned int i = 0; i < edgeUtils.size(); i++) {
		if (edgeUtils[i] > edgeCaps[i]) {
			v++;
		}
	}

	return v;
}


void RoutingSolver::violationSvg(const std::string& fileName)
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

void RoutingSolver::toSvg(const std::string& fileName)
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

void RoutingSolver::reorderNets()
{

	sort(nets.begin(), nets.end(), [](const Net &n1, const Net &n2) {
		return n2.pins.size() > n1.pins.size();
// 		return n1.pinTourManhattan() > n2.pinTourManhattan();
	});
}

void RoutingSolver::solveRouting()
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

	decomposeNets(nets, useNetDecomposition);

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

void RoutingSolver::logViolationSvg()
{
	if(!emitSVG) return;
	if(!htmlLog) {
		htmlLog.reset(new std::ofstream("log.html"));
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
RoutingSolver::RoutingSolver(RoutingInst &inst)
: gx(inst.gx)
, gy(inst.gy)
, cap(inst.cap)
, nets(inst.nets)
, edgeCaps(inst.edgeCaps)
, inst(inst)
{

}

RoutingSolver::~RoutingSolver()
{
	if(htmlLog && htmlLog.unique() && emitSVG)
		*htmlLog << "</body></html>";
}


namespace
{
	volatile std::sig_atomic_t signalHandlerCalls = 0;
	
	void handler(int n)
	{
		signalHandlerCalls++;
		std::signal(n, SIG_DFL);
	}
	
	struct SetHandler
	{
		int signalNumber;
		
		SetHandler(int signalNumber)
		: signalNumber(signalNumber)
		{
			std::signal(signalNumber, &handler);
		}
		
		~SetHandler()
		{
			std::signal(signalNumber, SIG_DFL);
		}
	};
}

void RoutingSolver::rrRoute()
{
	using std::chrono::steady_clock;

	
	const auto procedureStartTime = steady_clock::now();
	
	// get initial solution
	cout << "[1/2] Creating initial solution...\n";
	solveRouting();

	// rrr
	if(!useNetOrdering && !useNetDecomposition) return;
	cout << "[2/2] Rip up and reroute...\n";
	
	
	int deltaPenalty = 2;
	int deltaViolation = 0;
	int lastViolation = 0;
	const time_t startTime = time(nullptr);
	
	for(int iter = 0; true /* no iteration limit */; ++iter) {
		if(steady_clock::now() >= procedureStartTime + timeLimit) {
			cout << "Terminating due to expiration of time limit. Total time taken: " 
				<< chrono::duration_cast<chrono::seconds>(steady_clock::now() - procedureStartTime).count()
				<< " seconds.\n";
			break;
		}
		
		if(signalHandlerCalls > 0) {
			cout << "Terminating due to interrupt.\n";
			break;
		}
		cout << "--> Iteration " << iter << "\n";

		updateEdgeWeights();

		int violations = 0;
		for(auto &n : nets) if(hasViolation(n)) violations++;
		if(iter == 0) lastViolation = violations;
		deltaViolation = -lastViolation + violations;


		if(deltaViolation > 0) {
			penalty += deltaPenalty;
		} else {
			penalty -= deltaPenalty;
		}
		

		lastViolation = violations;

		if(useNetOrdering) {
			sort(nets.begin(), nets.end(), [&](const Net &n1, const Net &n2) {
				return totalEdgeWeight(n1) * netSpan(n2) > totalEdgeWeight(n2) * netSpan(n1);
			});
		}
		SetHandler signalHandlerSetting(SIGINT);
		ProgressBar pbar(cout);
		pbar.max = nets.size();
		PeriodicRunner<chrono::milliseconds> printer(chrono::milliseconds(200));
		int netsConsidered = 0;
		int netsRerouted = 0;

		auto printFunc = [&]()
		{
			pbar.value = netsConsidered;
			
			auto elap = steady_clock::now() - procedureStartTime;
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
			if(signalHandlerCalls > 0) {
				pbar
					.writeln("\033[31m^C pressed. Will write output and terminate at end of current RRR iteration.")
					.writeln("To stop immediately without writing output press ^C again.\033[0m");
			}
		};

		decomposeNets(nets, useNetDecomposition);

		for(auto &n : nets) {
			if(hasViolation(n)) {
				ripNet(n);
				routeNet(n);

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


std::ostream &operator <<(std::ostream &os, const Point &p)
{
	return os << "Point{" << p.x << ", " << p.y << "}";
}

std::ostream &operator <<(std::ostream &os, const Edge &e)
{
	return os << "Edge{" << e.p1 << ", " << e.p2 << "}";
}
