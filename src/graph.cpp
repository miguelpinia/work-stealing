#include "ws/lib.hpp"

///////////////////////////
// Vertex implementation //
///////////////////////////

vertex::vertex() : directed(false), value(0) {}

vertex::vertex(bool directed, int value) : directed(directed), value(value) {}

bool vertex::isDirected() const { return directed; }

int vertex::getValue() const { return value; }

void vertex::setNeighbours(std::list<int> &neighbours) {
    this->neighbours_ = neighbours;
}

std::list<int> &vertex::getNeighbours() {
    return neighbours_;
}

void vertex::setChildren(std::list<int> &children) {
    this->children_ = children;
}

std::list<int> &vertex::getChildren() {
    return children_;
}

void vertex::addNeighbour(int neighbour) {
    neighbours_.push_back(neighbour);
}

void vertex::deleteNeighbour(int neighbour) {
    bool toDelete = false;
    auto itr = neighbours_.begin();
    for (; itr != neighbours_.end(); itr++) {
        if (*itr == neighbour) {
            toDelete = true;
            break;
        }
    }
    if (toDelete) neighbours_.erase(itr);
}

void vertex::addChild(int child) {
    children_.push_back(child);
}

void vertex::deleteChild(int child) {
    auto itr = children_.begin();
    for (; itr != children_.end(); itr++) {
        if (*itr == child) break;
    }
    children_.erase(itr);
}

/////////////////////////
// Edge implementation //
/////////////////////////

edge::edge() : src(0), dest(0) {}

edge::edge(int src, int dest) : src(src), dest(dest) {}

int edge::getSrc() const { return src; }

int edge::getDest() const { return dest; }

//////////////////////////
// Graph implementation //
//////////////////////////

graph::graph() : directed(false), root(0), numEdges(0), numVertices(10), type(GraphType::RANDOM) {}

graph::graph(std::vector<edge> const edges, bool directed, int root, int numVertices, GraphType type) : directed(directed),
                                                                                                        root(root),
                                                                                                        numEdges(0),
                                                                                                        numVertices(numVertices),
                                                                                                        type(type) {
    for (auto i = 0; i < numVertices; i++) vertices.emplace_back(vertex(directed, i));
    for (auto edge: edges) addEdge(edge);
}

graph::graph(bool directed, int root, int numVertices, GraphType type) : directed(directed),
                                                                         root(root), numEdges(0),
                                                                         numVertices(numVertices),
                                                                         type(type) {
    for (int i = 0; i < numVertices; i++) vertices.emplace_back(vertex(directed, i));
}

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

void graph::addEdge(edge edge) {
    int src = edge.getSrc();
    int dest = edge.getDest();
    addEdge(src, dest);
}

void graph::addEdges(std::vector<edge> const edges) {
    for (auto edge: edges) {
        addEdge(edge);
    }
}

void graph::deleteEdge(edge edge) {
    int src = edge.getSrc();
    int dst = edge.getDest();
    vertices[src].deleteNeighbour(dst);
    vertices[dst].deleteNeighbour(src);
    numEdges--;
}

bool graph::hasEdge(edge edge) {
    int src = edge.getSrc();
    int dst = edge.getDest();
    std::list<int> neighbours = vertices[src].getNeighbours();
    bool found = std::find(neighbours.begin(),
                           neighbours.end(),
                           dst)
                 != neighbours.end();
    return found;
}

std::list<int> &graph::getNeighbours(int vertex) {
    return vertices[vertex].getNeighbours();
}

std::list<int> &graph::getChildren(int vertex) {
    return vertices[vertex].getChildren();
}

int graph::getRoot() const {
    return root;
}

int graph::getNumberVertices() const {
    return numVertices;
}

int graph::getNumberEdges() const {
    return numEdges;
}

GraphType graph::getType() {
    return type;
}

bool graph::isDirected() {
    return directed;
}

void graph::setDirected(bool directed) {
    this->directed = directed;
}
