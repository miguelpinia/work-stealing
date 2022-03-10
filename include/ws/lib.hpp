#pragma once

#include <list>

int suma(int a, int b);

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
