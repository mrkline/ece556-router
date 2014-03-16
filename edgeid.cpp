
#include "edgeid.hpp"
#include "ece556.hpp"
#include <set>
#include <iostream>

int horizontalEdgeID(int width, int height, int x, int y);
int verticalEdgeID(int width, int height, int x, int y);

int horizontalEdgeID(int width, int height, int x, int y)
{
	(void)height;
	return (width - 1) * y + x;
}


int verticalEdgeID(int width, int height, int x, int y)
{
	return (width - 1) * height + width * y + x;
}

int edgeID(int width, int height, int x1, int y1, int x2, int y2)
{
	if(x1 == x2) {
		assert(abs(y2 - y1) == 1);
		return verticalEdgeID(width, height, x1, std::min(y1, y2));
	}
	else if(y1 == y2) {
		assert(abs(x2 - x1) == 1);
		return horizontalEdgeID(width, height, std::min(x1, x2), y1);
	}
	else {
		assert(false && "edgeID: edges must be horizontal or vertical");
		return -1;
	}
}

int edgeID(int width, int height, const Edge &e)
{
	return edgeID(width, height, e.p1.x, e.p1.y, e.p2.x, e.p2.y);
}


Edge edge(int width, int height, int edgeID)
{
	int minVert = (width - 1) * height;
	bool isVert = edgeID >= minVert;
	if(isVert) edgeID -= minVert;


	if(isVert) { // vertical
		int x = edgeID % (width);
		int y = edgeID / (width);
		return {{x,y}, {x, y+1}};
	}
	else { // horizontal
		int x = edgeID % (width - 1);
		int y = edgeID / (width - 1);
		return {{x, y}, {x+1, y}};
	}
}

void testEdgeID()
{
	const int width = 4, height = 6;

	std::set<int> seen;

	for(int y = 0; y < height; ++y) {
		for(int x = 0; x < width; ++x) {

			if(x < width - 1) {
				auto horiz = Edge::horizontal({x, y});
				int horizID = edgeID(width, height, horiz);
				auto reconHorizEdge = edge(width, height, horizID);
				seen.insert(horizID);
				assert(reconHorizEdge == horiz);
				std::cout << horiz << " -> " << horizID << " -> " << reconHorizEdge << "\n";
			}



			if(y < height - 1) {
				auto vert = Edge::vertical({x, y});
				int vertID = edgeID(width, height, vert);
				auto reconVertEdge = edge(width, height, vertID);
				seen.insert(vertID);
				assert(reconVertEdge == vert);
	
				std::cout << vert << " -> " << vertID << " -> " << reconVertEdge << "\n";
			}
		}
	}
}