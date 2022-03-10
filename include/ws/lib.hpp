#pragma once

#include <list>
#include <vector>

int suma(int a, int b);

enum class GraphType { TORUS_2D, TORUS_2D_60, TORUS_3D, TORUS_3D_40, RANDOM, KGRAPH };

class vertex
{
private:
    bool directed;
    int value;
    std::list<int> neighbours;
    std::list<int> childs;
public:
    vertex();
    vertex(bool directed, int value);
    bool const& isDirected() const;
    int const& getValue() const;
    // I want to return a constant list that shouldnt be modified.
    void setNeighbours(std::list<int>& neighbours);
    std::list<int>& getNeighbours();
    void setChilds(std::list<int>& childs);
    std::list<int>& getChilds();

    void addNeighbour(int neighbour);
    void deleteNeighbour(int neighbour);
    void addChild(int neighbour);
    void deleteChild(int neighbour);
};

class edge
{
private:
    int src;
    int dest;
public:
    edge();
    edge(int src, int dest);
    int getSrc();
    int getDest();
};


class graph
{
private:
    std::vector<vertex> vertices;
    bool directed;
    int root;
    int numEdges;
    int numVertices;
    GraphType type;
public:
    graph();
    graph(std::vector<edge> edges, bool directed, int root, int numVertices, GraphType type);
    // graph(bool directed, int root, int numVertices, GraphType type);
    ~graph();
    void addEdge(int src, int dest);
    void addEdge(edge edge);
    void addEdges(std::vector<edge> edges);
    void deleteEdge(edge edge);
    bool hasEdge(edge edge);
    std::list<int>& getNeighbours(int vertex);
    std::list<int>& getChilds(int vertex);
    int getRoot();
    int getNumberVertices();
    int getNumberEdges();
    GraphType getType();
    void setDirected(bool directed);
    bool isDirected();
};
