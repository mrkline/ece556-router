/// \file
#ifndef EDGEID_HPP_NK084F
#define EDGEID_HPP_NK084F

#include <cassert>
#include <cmath>
#include <algorithm>

struct Edge;

/// Convert a line segment of length one into an edge ID.
/// \param width   The number of vertical divisions in the grid.
/// \param height  The number of horizontal divisions in the grid.
int edgeID(int width, int height, int x1, int y1, int x2, int y2);

/// \overload edgeID(int width, int height, int x1, int y1, int x2, int y2)
int edgeID(int width, int height, const Edge &);

/// Convert an edge ID back into an Edge.
/// \sa edgeID(int width, int height, int x1, int y1, int x2, int y2)
Edge edge(int width, int height, int edgeID);

void testEdgeID();

#endif // EDGEID_HPP_NK084F