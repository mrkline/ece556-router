#ifndef ROUTINGSOLVER_HPP_WSYRWD
#define ROUTINGSOLVER_HPP_WSYRWD

#include <memory>
#include <vector>
#include "ece556.hpp"


/// Solves a routing instance
struct RoutingSolver {
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
	int netSpan(const Net &n) const;
	bool hasViolation(const Net &n) const;

	int penalty = 10;

public:
	RoutingSolver();
	~RoutingSolver();
	bool emitSVG = false;
	std::chrono::seconds timeLimit = std::chrono::seconds::max();
	
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
void readBenchmark(const char *fileName, RoutingSolver& rst);


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

