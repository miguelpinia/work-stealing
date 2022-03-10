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
