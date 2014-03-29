

#include "writer.hpp"


void Writer::write(const Point &p)
{
	out << "(" << p.x << "," << p.y << ")";
}

void Writer::write(const Edge &e)
{
	write(e.p1);
	out << "-";
	write(e.p2);
	out << "\n";
}

void Writer::write(const Net &n)
{
	out << "n" << n.id << "\n";
	for(const auto &segment : n.nroute) {
		for(int edge : segment.edges) {
			write(routing.edge(edge));
		}
	}
	out << "!\n";
}

void Writer::writeRouting()
{
	for(const auto &net : routing.nets) {
		write(net);
	}
}