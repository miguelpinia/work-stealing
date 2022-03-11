#include "ws/lib.hpp"
#include <algorithm>

int suma(int a, int b)
{
    return a + b;
}

///////////////////////////
// Vertex implementation //
///////////////////////////

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
    bool toDelete = false;
    auto itr = neighbours.begin();
    for (; itr != neighbours.end(); itr++) {
        if (*itr == neighbour) {
            toDelete = true;
            break;
        }
    }
    if (toDelete) neighbours.erase(itr);
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

/////////////////////////
// Edge implementation //
/////////////////////////

edge::edge() : src(0), dest(0) {}
edge::edge(int src, int dest) : src(src), dest(dest) {}

int edge::getSrc() { return src; }
int edge::getDest() { return dest; }

//////////////////////////
// Graph implementation //
//////////////////////////

graph::graph() : directed(false), root(0), numEdges(0), numVertices(10), type(GraphType::RANDOM)
{}

graph::graph(std::vector<edge> edges, bool directed, int root, int numVertices, GraphType type) : directed(directed), root(root), numEdges(0), numVertices(numVertices), type(type)
{
    for (int i = 0; i < numVertices; i++) {
        vertices.push_back(vertex(directed, i));
    }
    for (auto edge: edges) {
        addEdge(edge);
    }
}

graph::~graph() {}

void graph::addEdge(int src, int dest) {
    if (dest == -1) return;
    vertices[src].addNeighbour(dest);
    numEdges++;
    if (directed) {
        vertices[dest].addChild(src);
    } else {
        vertices[dest].addNeighbour(src);
        numEdges++;
    }
}

void graph::addEdge(edge edge)
{
    int src = edge.getSrc();
    int dest = edge.getDest();
    addEdge(src, dest);
}

void graph::addEdges(std::vector<edge> edges)
{
    for (auto edge: edges) {
        addEdge(edge);
    }
}

void graph::deleteEdge(edge edge)
{
    int src = edge.getSrc();
    int dst = edge.getDest();
    vertices[src].deleteNeighbour(dst);
    vertices[dst].deleteNeighbour(src);
    numEdges--;
}

bool graph::hasEdge(edge edge)
{
    int src = edge.getSrc();
    int dst = edge.getDest();
    std::list<int> neighbours = vertices[src].getNeighbours();
    bool found = (std::find(neighbours.begin(),
                            neighbours.end(),
                            dst)
                  != neighbours.end());
    return found;
}

std::list<int>& graph::getNeighbours(int vertex)
{
    return vertices[vertex].getNeighbours();
}

std::list<int>& graph::getChilds(int vertex)
{
    return vertices[vertex].getChilds();
}

int graph::getRoot()
{
    return root;
}

int graph::getNumberVertices()
{
    return numVertices;
}

int graph::getNumberEdges()
{
    return numEdges;
}

GraphType graph::getType()
{
    return type;
}

bool graph::isDirected()
{
    return directed;
}

void graph::setDirected(bool directed)
{
    this->directed = directed;
}

//////////////////////////
// Task array with size //
//////////////////////////

taskArrayWithSize::taskArrayWithSize() {}
taskArrayWithSize::taskArrayWithSize(int size) : size(size)
{
    array = std::vector<int>(size, -1);
}

int& taskArrayWithSize::getSize()
{
    return size;
}

int& taskArrayWithSize::get(int position)
{
    return array[position];
}

void taskArrayWithSize::set(int position, int value)
{
    array[position] = value;
}
