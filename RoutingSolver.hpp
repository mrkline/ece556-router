#ifndef ROUTINGSOLVER_HPP_WSYRWD
#define ROUTINGSOLVER_HPP_WSYRWD

#include <memory>
#include <vector>
#include <set>
#include <unordered_set>
#include <unordered_map>
#include "ece556.hpp"
#include "RoutingInst.hpp"
#include "options.hpp"

void decomposeNets(std::vector<Net>& nets, bool useNetDcomposition);

void decomposeNetSimple(Net &n);
void decomposeNetMST(Net &n);
void decomposeNet(Net& n, bool useNetDecomposition);

/// Solves a routing instance
class RoutingSolver {

	struct EdgeInfo
	{
		int overflowCount; // k_e^k
		int weight; // w_e^k

		std::set<int> nets;
		
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
	int netSpan(const Net &n) const;
	int netOverlap(const Net &m, const Net &n) const;
	int netOverlapArea(const Net &m, const Net &n) const;
	int netArea(const Net &n) const;

	bool hasViolation(const Net &n) const;

	int penalty = 20;

	int edgeID(const Point &p1, const Point &p2) const
	{
		return inst.edgeID(p1, p2);
	}

	int edgeID(const Edge &e) const
	{
		return inst.edgeID(e);
	}
	
	Edge edge(int edgeID) const
	{
		return inst.edge(edgeID);
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

	int edgeUtil(const Edge &e) const
	{
		return edgeUtil(e.p1, e.p2);
	}

	void setEdgeCap(const Point &p1, const Point &p2, int capacity)
	{
		inst.setEdgeCap(p1, p2, capacity);
	}

	int edgeCap(const Point &p1, const Point &p2) const
	{
		return inst.edgeCap(p1, p2);
	}

	int edgeCap(const Edge &e) const
	{
		return edgeCap(e.p1, e.p2);
	}

	int gx; ///< x dimension of the global routing grid
	int gy; ///< y dimension of the global routing grid

	int cap;
	int iteration = 1;

	std::vector<Net> &nets;
	std::vector<Net *> nets_byid;

	int numEdges; ///< number of edges of the grid
	std::vector<int> &edgeCaps; ///< array of the actual edge capacities after considering for blockage
	std::vector<int> edgeUtils; ///< array of edge utilizations
	void logViolationSvg();
public:
	Options::CostFunction costFunction = Options::Standard;
	RoutingInst &inst;
	bool emitSVG = false;
	std::chrono::seconds timeLimit = std::chrono::seconds::max();

	bool useNetDecomposition = true;
	bool useNetOrdering = true;
	bool findDependencyChains = false;

	RoutingSolver(RoutingInst &inst);
	~RoutingSolver();

	bool neighbor(Point &p, unsigned int caseNumber);


	void reorderNets(std::vector<Net>& nets);
	void reorderNetsFancy(std::vector<Net>& nets);

	/// Use A* search to route a segment with the overflow 
	/// penalty from the member variable `penalty`.
	void aStarRouteSeg(Path& s);

	// L-shaped routing
	void connectViaLine(std::vector<int>& s, Point p0, Point p1);
	void ellRouteSeg(Path& s);
	
	void routeNet(Net& n);
	void placeNet(const Net& n);
	void ripNet(Net& n);
	int countViolations();
	bool routeValid(Route& r, bool isplaced);


	void violationSvg(const std::string& fileName);
	void toSvg(const std::string& fileName);
	void violationTxt(const std::string &filename);
	
	void solveRouting();
	void rrr();
};


/**
 * \brief reates a routing solution
 * \param rst The routing instance
 */
void solveRouting(RoutingSolver& rst);

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
void writeOutput(const char *outRouteFile, RoutingSolver& rst);

#endif // ROUTINGSOLVER_HPP_WSYRWD

