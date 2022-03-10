#include "ws/lib.hpp"

int suma(int a, int b)
{
    return a + b;
}

vertex::vertex() : directed(false), value(0)
{}

vertex::vertex(bool directed, int value) : directed(directed), value(value)
{}

bool const& vertex::isDirected() const { return directed; }


int const& vertex::getValue() const { return value; }

void vertex::setNeighbours(std::list<int>& neighbours)
{
    this->neighbours = neighbours;
}

std::list<int>& vertex::getNeighbours()
{
    return neighbours;
}

void vertex::setChilds(std::list<int>& childs)
{
    this->childs = childs;
}

std::list<int>& vertex::getChilds()
{
    return childs;
}

void vertex::addNeighbour(int neighbour)
{
    neighbours.push_back(neighbour);
}

void vertex::deleteNeighbour(int neighbour)
{
    auto itr = neighbours.begin();
    for (; itr != neighbours.end(); itr++) {
        if (*itr == neighbour) {
            break;
        }
    }
    neighbours.erase(itr);
}

void vertex::addChild(int child)
{
    childs.push_back(child);
}

void vertex::deleteChild(int child)
{
    auto itr = childs.begin();
    for(; itr != childs.end(); itr++) {
        if (*itr == child) break;
    }
    childs.erase(itr);
}
