
#ifndef ROUTINGINST_HPP_OP8EQ1
#define ROUTINGINST_HPP_OP8EQ1

#include <vector>
#include "ece556.hpp"


struct RoutingInst {
	int gx, gy;
	int cap;
	std::vector<int> edgeCaps;
	std::vector<Net> nets;


	void setEdgeCap(const Point &p1, const Point &p2, int capacity)
	{
		int id = edgeID(p1, p2);
		getElementResizingIfNecessary(edgeCaps, id, cap) = capacity;
	}

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

	int edgeCap(const Point &p1, const Point &p2) const
	{
		return getElementOrDefault(edgeCaps, edgeID(p1, p2), cap);
	}

};

#endif // ROUTINGINST_HPP_OP8EQ1

