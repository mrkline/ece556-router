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
#include <map>
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

namespace
{
	ofstream &getTextLogStream()
	{
		static ofstream result("text_index.txt");
		return result;
	}
	
}

void RoutingSolver::updateEdgeWeights()
{
	edgeInfos.resize(edgeCaps.size());

	for(size_t i = 0; i < edgeInfos.size(); ++i) {
		auto &ei = edgeInfos[i];
		const int overflow = edgeUtils[i] - edgeCaps[i];

		if(overflow > 0) {
			ei.overflowCount++;
			ei.weight = overflow * ei.overflowCount;
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


int RoutingSolver::netOverlapArea(const Net &m, const Net &n) const
{
	int mxmax = 0, mymax = 0;
	int mxmin = gx, mymin = gy;
	int nxmax = 0, nymax = 0;
	int nxmin = gx, nymin = gy;
	int sxmax, symax, sxmin, symin;
	for (const auto &p : m.pins) {
		mxmax = std::max<int>(mxmax, p.x);
		mymax = std::max<int>(mymax, p.y);
		mxmin = std::min<int>(mxmin, p.x);
		mymin = std::min<int>(mymin, p.y);
	}
	for (const auto &p : n.pins) {
		nxmax = std::max<int>(nxmax, p.x);
		nymax = std::max<int>(nymax, p.y);
		nxmin = std::min<int>(nxmin, p.x);
		nymin = std::min<int>(nymin, p.y);
	}
	sxmax = std::min<int>(nxmax, mxmax);
	sxmin = std::max<int>(nxmin, mxmin);
	symax = std::min<int>(nymax, mymax);
	symin = std::max<int>(nymin, mymin);

	if (sxmax <= sxmin || symax <= symin) {
		return 0;
	}

	return (sxmax - sxmin) * (symax - symin);
}

int RoutingSolver::netOverlap(const Net &m, const Net &n) const
{
	int mxmax = 0, mymax = 0;
	int mxmin = gx, mymin = gy;
	int nxmax = 0, nymax = 0;
	int nxmin = gx, nymin = gy;
	int sxmax, symax, sxmin, symin;
	for (const auto &p : m.pins) {
		mxmax = std::max<int>(mxmax, p.x);
		mymax = std::max<int>(mymax, p.y);
		mxmin = std::min<int>(mxmin, p.x);
		mymin = std::min<int>(mymin, p.y);
	}
	for (const auto &p : n.pins) {
		nxmax = std::max<int>(nxmax, p.x);
		nymax = std::max<int>(nymax, p.y);
		nxmin = std::min<int>(nxmin, p.x);
		nymin = std::min<int>(nymin, p.y);
	}
	sxmax = std::min<int>(nxmax, mxmax);
	sxmin = std::max<int>(nxmin, mxmin);
	symax = std::min<int>(nymax, mymax);
	symin = std::max<int>(nymin, mymin);
	int c = 0;
	for (const auto &p : m.pins) {
		if (p.x <= sxmax && p.x >= sxmin && p.y <= symax && p.y >= symin) {
			c++;
		}
	}
	return c;
}


int RoutingSolver::netArea(const Net &n) const
{
	int xmax = 0, ymax = 0;
	int xmin = gx, ymin = gy;
	for(const auto &p : n.pins) {
		xmax = std::max<int>(xmax, p.x);
		ymax = std::max<int>(ymax, p.y);
		xmin = std::min<int>(xmin, p.x);
		ymin = std::min<int>(ymin, p.y);
	}
	return (xmax - xmin) * (ymax - ymin);
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

void RoutingSolver::connectViaLine(std::vector<int>& s, Point p1, Point p2) {
	if (p1.x == p2.x) {
		Point p = Point{p1.x, std::min<int>(p1.y, p2.y)};
		int end = std::max<int>(p1.y, p2.y);
		for (; p.y < end; p.y++) {
			s.push_back(edgeID(Edge::vertical(p)));
		}
	} else if (p1.y == p2.y) {
		Point p = Point{std::min<int>(p1.x, p2.x), p1.y};
		int end = std::max<int>(p1.x, p2.x);
		for (; p.x < end; p.x++) {
			s.push_back(edgeID(Edge::horizontal(p)));
		}
	} else {
		assert (0 == 1);
	}
}

void RoutingSolver::ellRouteSeg(Path& s)
{
	int xyv = 0, yxv = 0;
	std::vector<int> xyr, yxr;


	connectViaLine(xyr, s.p1, Point{s.p1.x, s.p2.y});
	connectViaLine(xyr, s.p2, Point{s.p1.x, s.p2.y});

	connectViaLine(yxr, s.p1, Point{s.p2.x, s.p1.y});
	connectViaLine(yxr, s.p2, Point{s.p2.x, s.p1.y});

	for (const auto e : xyr) {
		xyv += 1 + penalty*getElementOrDefault(edgeUtils, e, 0) /
			(getElementOrDefault(edgeCaps, e, cap) + 1);
	}

	for (const auto e : yxr) {
		yxv += 1 + penalty*getElementOrDefault(edgeUtils, e, 0) /
			(getElementOrDefault(edgeCaps, e, cap) + 1);
	}

	if (yxv < xyv) {
		s.edges = yxr;
	} else {
		s.edges = xyr;
	}
}

void RoutingSolver::aStarRouteSeg(Path& s)
{
	unordered_set<Point> open;
	unordered_set<Point> closed;
	typedef std::pair<double, Point> CostPoint;
	
	auto costComp = [&](const CostPoint &p1, const CostPoint &p2) {
		return p1.first + s.p2.l1dist(p1.second) >
			p2.first + s.p2.l1dist(p2.second);
	};
	
	priority_queue<CostPoint, vector<CostPoint>, decltype(costComp)> open_score(costComp);
	unordered_map<Point, Point> prev;

	Point p0;
	double p0_cost;

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
			
			double extraCost;
			if(costFunction == Options::NC) {
				double h = min(1.0, 0.5 + iteration / 100.0);
				double k = min(1.0, 0.01 + iteration / 100.0);
				auto &ei = getElementResizingIfNecessary(edgeInfos, edgeID(p, p0), {});
				extraCost = 1 + h / (1.0 + exp(-k * (edgeUtil(p, p0) + ei.weight) / edgeCap(p, p0))) - h;
			}
			else {
				extraCost = 1 + penalty * edgeUtil(p, p0) / (edgeCap(p, p0) + 1);
			}
			// queue valid neighbors for future examination
			open.emplace(p);
			open_score.emplace(
				p0_cost + extraCost,
// 				p0_cost + 1 +
// 				penalty*edgeUtil(p, p0) / (edgeCap(p, p0)+1),
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
	parallelForEach(n.nroute.begin(), n.nroute.end(), 
	                [&](Path &path) { aStarRouteSeg(path); });
}

void RoutingSolver::placeNet(const Net& n)
{
	unordered_set<int> placed;

	for (const auto s : n.nroute) {
		for (const auto edge : s.edges) {
			if (placed.count(edge) > 0) {
				continue;
			}

			if (findDependencyChains) {
				auto &ei = getElementResizingIfNecessary(edgeInfos, edge, EdgeInfo{});
				ei.nets.emplace(n.id);
			}

			getElementResizingIfNecessary(edgeUtils, edge, 0)++;
			placed.emplace(edge);
		}
	}
}

// rip up the route from an old net and return it
void RoutingSolver::ripNet(Net& n)
{
	unordered_set<int> ripped;

	for (const auto &s : n.nroute) {
		for (const auto edge : s.edges) {
			if (ripped.count(edge) > 0) {
				continue;
			}

			if (findDependencyChains) {
				auto &ei = getElementResizingIfNecessary(edgeInfos, edge, EdgeInfo{});
				ei.nets.erase(n.id);
			}

			getElementResizingIfNecessary(edgeUtils, edge, 0)--;
			ripped.emplace(edge);
		}
	}
	n.nroute.clear();
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


void RoutingSolver::reorderNets(std::vector<Net>& nets)
{
	// this needs to be a total order
	sort(nets.begin(), nets.end(), [&](const Net &n1, const Net &n2) {
		int o, s1, s2;
		o = netOverlapArea(n1, n2) * netOverlap(n1, n2);
		s1 = netArea(n1) * n1.pins.size();
		s2 = netArea(n2) * n2.pins.size();
		return (s1 - s2) * (o * o + 1) < 0;
	});
}

void RoutingSolver::reorderNetsFancy(std::vector<Net>& nets)
{
	std::cout << "start reorder\n";

	std::set<int> worst_nets;
	
	auto edgeComp = [&](const int e1, const int e2) {
		//return edgeInfos[e1].nets.size() < edgeInfos[e2].nets.size();
		return edgeUtils[e1] * (edgeCaps[e2] + 1) < edgeUtils[e2] * (edgeCaps[e1] + 1);
	};

	// find ~1000 nets on the worst edges
	std::priority_queue<int, std::vector<int>, decltype(edgeComp)> q(edgeComp);
	for (unsigned int e = 0; e < edgeInfos.size(); e++) {
		q.emplace(e);
	}
	while (worst_nets.size() < 5000 && !q.empty()) {
		for (const auto n : edgeInfos[q.top()].nets) {
			assert(edgeInfos[q.top()].nets.size() != 0);
			worst_nets.insert(n);
		}
		q.pop();
	}

	struct ChainLength {
		RoutingSolver &rs;
		std::set<int> ids;
		std::vector<int> id_vec;

		std::map<int, int> chainlen;
		std::unordered_map<int, int> span;
		std::unordered_map<int, int> computed;
		std::unordered_map<int, std::map<int, int>> overlap;

		ChainLength(RoutingSolver &r, std::set<int> &nets) : rs(r), ids(nets) {
			for (const int id : ids) {
				span[id] = rs.netSpan(*rs.nets_byid[id]);
				id_vec.push_back(id);
			}

			// compute pairwise overlap of nets
			for (unsigned int e = 0; e < rs.edgeInfos.size(); e++) {
				for (unsigned int i = 0; i < id_vec.size(); i++) {
					if (!rs.edgeInfos[e].nets.count(i)) continue;
					for (unsigned int j = i + 1; j < id_vec.size(); j++) {
						if (!rs.edgeInfos[e].nets.count(j)) continue;
						overlap[id_vec[i]][id_vec[j]]++;
						overlap[id_vec[j]][id_vec[i]]++;
					}
				}
			}

			// compute all chain lengths
			for (const int id : ids) {
				chainLength(id);
			}
			reorderOrder();

			std::cout << "\nchain lengths: ";
			for (const int id : id_vec) {
				std::cout << chainlen[id] << ", ";
			}
			std::cout << "\ndone computing chainlengths...\n";

			// WARNING: WEIRD AND CRAZY THING!!!!
			// pull up the most obnoxious nets so nothing has to reroute over them.
			for (const int id : id_vec) {
				rs.ripNet(*rs.nets_byid[id]);
			}
		}

		// find the length of the longest chain with "id" as minimal element
		// TODO: ... or do I want it as the maximal element??
		int chainLength(int id) {
			if (computed[id] == 1) {
				return chainlen[id];
			} else if (computed[id] == -1) {
				// cycle warning!!! Shouldn't happen (I hope)
				std::cout << "!";
				return 0;
			}
	
			// initialize computation for this chain
			computed[id] = -1;
			chainlen[id] = 0;

			// find chain length based on dominating nets
			for (const int jd : ids) {
				int o = overlap[id][jd];
				if (id == jd || !o || span[id] >= span[jd]) continue;
	
				// find max chain length for non-dominated nets
				chainlen[id] = std::max<int>(chainlen[id], chainLength(jd) + 1);
			}
			computed[id] = 1;
			return chainlen[id];
		}

		void reorderOrder() {
			stable_sort(rs.nets.begin(), rs.nets.end(), [&](const Net &n1, const Net &n2) {
				if (!ids.count(n1.id) && !ids.count(n2.id)) {
					return false;
				} else if (!ids.count(n1.id)) {
					return true;
				} else if (!ids.count(n2.id)) {
					return false;
				} else {
					return chainlen[n1.id] < chainlen[n2.id];
				}
			});
		}
	} cl (*this, worst_nets);
}

chrono::time_point<chrono::steady_clock> procedureStartTime;

void RoutingSolver::solveRouting()
{
	cout << "[1/2] Creating initial solution...\n";

	int startTime = time(0);
	procedureStartTime = chrono::steady_clock::now();

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
	
	*htmlLog << "<div class=\"background\"><img src=\"" << filename << "\"></div>\n" << std::flush;
	std::cout << "violations logged to [" << filename << "]\n";

	violationSvg(filename);
	
	ss.str("");
	ss << "violations_" << timeString.data() << ".txt";
	filename = ss.str();
	
	violationTxt(filename);
	getTextLogStream() << filename << endl;
}

void RoutingSolver::violationTxt(const string &filename)
{
	ofstream txt(filename);
	
	Point p;
	for(p.y = 0; p.y < gy; ++p.y)
	{
		for(p.x = 0; p.x < gx; ++p.x)
		{
			auto horizEdge = Edge::horizontal(p);
			auto vertEdge = Edge::vertical(p);
			int ovf = edgeUtil(horizEdge) + edgeUtil(vertEdge) - edgeCap(horizEdge) - edgeCap(vertEdge);
			txt << ovf << ' ';
		}
		txt << '\n';
	}
}

RoutingSolver::RoutingSolver(RoutingInst &inst)
: gx(inst.gx)
, gy(inst.gy)
, cap(inst.cap)
, nets(inst.nets)
, edgeCaps(inst.edgeCaps)
, inst(inst)
{
	for (unsigned int i = 0; i < inst.nets.size(); i++) {
		nets_byid.push_back(&inst.nets[i]);
		if (i != (unsigned int)inst.nets[i].id) {
			std::cout << "nets aren't ordered!!!\n";
			exit(-1);
		}
	}
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

void RoutingSolver::rrr()
{
	using std::chrono::steady_clock;

	cout << "[2/2] Rip up and reroute...\n";
	
	
	int deltaPenalty = 1;
	int deltaViolation = 0;
	int lastViolation = 0;
	penalty = 20;
	const time_t startTime = time(nullptr);
	
	for(int iter = 0; true /* no iteration limit */; ++iter) {
		++iteration;
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
			for(auto &net : nets) {
				net.totalEdgeWeight = totalEdgeWeight(net);
				net.netSpan = netSpan(net);
			}

			reorderNets(nets);
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

		for(auto &n : nets) {
			// always run for NC
			if(costFunction == Options::NC || hasViolation(n)) {

				ripNet(n);
				decomposeNet(n, useNetDecomposition);
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
