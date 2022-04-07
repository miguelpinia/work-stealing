#include "ws/lib.hpp"
#include <random>
#include <queue>
#include <cmath>

////////////////////
// Util functions //
////////////////////

int mod(int a, int b)
{
    return ((a % b) + b) % b;
}

graph torus2D(int shape)
{
    bool directed = false;
    int numEdges = shape * shape * 4;
    int numVertices = shape * shape;
    graph g(directed, 0, numVertices, GraphType::TORUS_2D);
    int neighbor;
    int i, j, currentIdx, pos;
    for(int k = 0; k < numEdges; k++) {
        j = mod(std::round(k / 4), shape);
        i = std::round(k / (shape * 4));
        currentIdx = (i * shape) + j;
        pos = mod(k, 4);
        switch(pos) {
        case 0:
            neighbor = mod((i - 1), shape) * shape + j;
            break;
        case 1:
            neighbor = i * shape + mod((j + 1), shape);
            break;
        case 2:
            neighbor = mod((i + 1), shape) * shape + j;
            break;
        case 3:
            neighbor = i * shape + mod((j - 1), shape);
            break;
        default:
            neighbor = 0;
        }
        g.addEdge(currentIdx, neighbor);
    }
    return g;
}

graph directedTorus2D(int shape)
{
    bool directed = true;
    int numVertices = shape * shape;
    int numEdges = numVertices * 2;
    graph g(directed, 0, numVertices, GraphType::TORUS_2D);
    int neighbor, i, j, currentIdx, pos;
    for (int k = 0; k < numEdges; k++) {
        j = mod((k / 2), shape);
        i = k / (shape * 2);
        currentIdx = (i * shape) + j;
        pos = mod(k, 2);
        switch(pos) {
        case 0:
            neighbor = i * shape + mod((j + 1), shape);
            break;
        case 1:
            neighbor = mod((i + 1), shape) * shape + j;
            break;
        default:
            neighbor = 0;
        }
        g.addEdge(currentIdx, neighbor);
    }
    return g;
}

graph torus2D60(int shape)
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distrib(0, 99);
    bool directed = false;
    int numVertices = shape * shape;
    int numEdges = numVertices * 4;
    graph g(directed, 0, numVertices, GraphType::TORUS_2D_60);
    int randomNumber, i, j, currentIdx, pos, neighbor;
    for (int k = 0; k < numEdges; k++) {
        j = mod(std::round(k / 4), shape);
        i = std::round(k / (shape * 4));
        currentIdx = (i * shape) + j;
        pos = mod(k, 4);
        randomNumber = distrib(gen);
        switch(pos) {
        case 0:
            neighbor = mod((i - 1), shape) * shape + j;
            g.addEdge(currentIdx, neighbor);
            break;
        case 1:
            if (randomNumber < 60) {
                neighbor = i * shape + mod((j + 1), shape);
                g.addEdge(currentIdx, neighbor);
            }
            break;
        case 2:
            if (randomNumber < 60) {
                neighbor = mod((i + 1), shape) * shape + j;
                g.addEdge(currentIdx, neighbor);
            }
            break;
        case 3:
            if (randomNumber < 60) {
                neighbor = i * shape + mod((j - 1), shape);
                g.addEdge(currentIdx, neighbor);
            }
            break;
        default: break;
        }
    }
    return g;
}

graph directedTorus2D60(int shape)
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distrib(0, 99);
    bool directed = true;
    int numVertices = shape * shape;
    int numEdges = numVertices * 2;
    graph g(directed, 0, numVertices, GraphType::TORUS_2D_60);
    int randomNumber, i, j, currentIdx, pos, neighbor;
    for (int k = 0; k < numEdges; k++) {
        j = mod(std::round(k / 2), shape);
        i = std::round(k / (shape * 2));
        currentIdx = (i * shape) + j;
        pos = mod(k, 2);
        randomNumber = distrib(gen);
        switch(pos) {
        case 0:
            neighbor = i * shape + mod((j + 1), shape);
            g.addEdge(currentIdx, neighbor);
            break;
        case 1:
            if (randomNumber < 60) {
                neighbor = mod((i + 1), shape) * shape + j;
                g.addEdge(currentIdx, neighbor);
            }
            break;
        default: break;
        }
    }
    return g;
}

graph torus3D(int shape)
{
    bool directed = false;
    int numVertices = shape * shape * shape;
    int numEdges = numVertices * 6;
    graph g(directed, 0, numVertices, GraphType::TORUS_3D);
    int i, j, k, currentIdx, pos, neighbor;
    for (int m = 0; m < numEdges; m++) {
        k = mod(std::round(m / 6), shape);
        j = mod(std::round(m / (shape * 6)), shape);
        i = mod(std::round(m / (shape * shape * 6)), shape);
        currentIdx = (i * shape * shape) + (j * shape) + k;
        pos = mod(m, 6);
        switch(pos) {
        case 0:
            neighbor = (i * shape * shape) + (j * shape) + mod((k - 1), shape);
            break;
        case 1:
            neighbor = (i * shape * shape) + (j * shape) + mod((k + 1), shape);
            break;
        case 2:
            neighbor = (i * shape * shape) + (mod((j - 1), shape) * shape) + k;
            break;
        case 3:
            neighbor = (i * shape * shape) + (mod((j + 1), shape) * shape) + k;
            break;
        case 4:
            neighbor = (mod((i - 1), shape) * shape * shape) + (j * shape) + k;
            break;
        case 5:
            neighbor = (mod((i + 1), shape) * shape * shape) + (j * shape) + k;
            break;
        default:
            neighbor = 0;
        }
        g.addEdge(currentIdx, neighbor);
    }
    return g;
}

graph directedTorus3D(int shape)
{
    bool directed = true;
    int numVertices = shape * shape * shape;
    int numEdges = numVertices * 3;
    graph g(directed, 0, numVertices, GraphType::TORUS_3D);
    int i, j, k, currentIdx, pos, neighbor;
    for (int m = 0; m < numEdges; m++) {
        k = mod(std::round(m / 3), shape);
        j = mod(std::round(m / (shape * 3)), shape);
        i = mod(std::round(m / (shape * shape * 3)), shape);
        currentIdx = (i * shape * shape) + (j * shape) + k;
        pos = mod(m, 3);
        switch(pos) {
        case 0:
            neighbor = (i * shape * shape) + (j * shape) + mod((k + 1), shape);
            break;
        case 1:
            neighbor = (i * shape * shape) + (mod((j + 1), shape) * shape) + k;
            break;
        case 2:
            neighbor = (mod((i + 1), shape) * shape * shape) + (j * shape) + k;
            break;
        default:
            neighbor = 0;
        }
        g.addEdge(currentIdx, neighbor);
    }
    return g;
}

graph torus3D40(int shape)
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distrib(0, 99);
    bool directed = false;
    int numVertices = shape * shape * shape;
    int numEdges = numVertices * 6;
    graph g(directed, 0, numVertices, GraphType::TORUS_3D_40);
    int i, j, k, currentIdx, pos, neighbor, randomNum;
    for (int m = 0; m < numEdges; m++) {
        k = mod(std::round(m / 6), shape);
        j = mod(std::round(m / (shape * 6)), shape);
        i = mod(std::round(m / (shape * shape * 6)), shape);
        currentIdx = (i * shape * shape) + (j * shape) + k;
        pos = mod(m, 6);
        randomNum = distrib(gen);
        switch (pos) {
        case 0:
            neighbor = (i * shape * shape) + (j * shape) + mod((k - 1), shape);
            g.addEdge(currentIdx, neighbor);
            break;
        case 1:
            if (randomNum < 40) {
                neighbor = (i * shape * shape) + (j * shape) + mod((k + 1), shape);
                g.addEdge(currentIdx, neighbor);
            }
            break;
        case 2:
            if (randomNum < 40) {
                neighbor = (i * shape * shape) + mod((j - 1), shape) * shape + k;
                g.addEdge(currentIdx, neighbor);
            }
            break;
        case 3:
            if (randomNum < 40) {
                neighbor = (i * shape * shape) + mod((j + 1), shape) * shape + k;
                g.addEdge(currentIdx, neighbor);
            }
            break;
        case 4:
            if (randomNum < 40) {
                neighbor = (mod((i - 1), shape) * shape * shape) + (j * shape) + k;
                g.addEdge(currentIdx, neighbor);
            }
            break;
        case 5:
            if (randomNum < 40) {
                neighbor = (mod((i + 1), shape) * shape * shape) + (j * shape) + k;
                g.addEdge(currentIdx, neighbor);
            }
            break;
        default:
            break;
        }
    }
    return g;
}

graph directedTorus3D40(int shape)
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distrib(0, 99);
    bool directed = true;
    int numVertices = shape * shape * shape;
    int numEdges = numVertices * 3;
    graph g(directed, 0, numVertices, GraphType::TORUS_3D_40);
    int i, j, k, currentIdx, pos, neighbor, randomNum;
    for (int m = 0; m < numEdges; m++) {
        k = mod(std::round(m / 3), shape);
        j = mod(std::round(m / (shape * 3)), shape);
        i = mod(std::round(m / (shape * shape * 3)), shape);
        currentIdx = (i * shape * shape) + (j * shape) + k;
        pos = mod(m, 3);
        randomNum = distrib(gen);
        switch (pos) {
        case 0:
            neighbor = (i * shape * shape) + (j * shape) + mod((k + 1), shape);
            g.addEdge(currentIdx, neighbor);
            break;
        case 1:
            if (randomNum < 40) {
                neighbor = (i * shape * shape) + mod((j + 1), shape) * shape + k;
                g.addEdge(currentIdx, neighbor);
            }
            break;
        case 2:
            if (randomNum < 40) {
                neighbor = (mod((i + 1), shape) * shape * shape) + (j * shape) + k;
                g.addEdge(currentIdx, neighbor);
            }
            break;
        default:
            break;
        }
    }
    std::string s;
    return g;
}

int pickRandomThread(int numThreads, int self) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distrib(0, numThreads - 1);
    int val = distrib(gen);
    return val == self ? mod(val + 1, numThreads) : val;
}


workStealingAlgorithm* workStealingAlgorithmFactory(AlgorithmType algType, int capacity, int numThreads)
{
    switch (algType) {
    case AlgorithmType::CILK:
        return new cilk(capacity);
    case AlgorithmType::CHASELEV:
        return new chaselev(capacity);
    case AlgorithmType::IDEMPOTENT_FIFO:
        return new idempotentFIFO(capacity);
    case AlgorithmType::IDEMPOTENT_LIFO:
        return new idempotentLIFO(capacity);
    case AlgorithmType::IDEMPOTENT_DEQUE:
        return new idempotentDeque(capacity);
    case AlgorithmType::WS_NC_MULT_OPT:
        return new wsncmult(capacity, numThreads);
    case AlgorithmType::B_WS_NC_MULT_OPT:
        return new bwsncmult(capacity, numThreads);
    case AlgorithmType::LAST:
        break;
        // case AlgorithmType::SIMPLE:
        //     break;
        // case AlgorithmType::WS_NC_MULT_LA_OPT:
        //     break;
        // case AlgorithmType::B_WS_NC_MULT_LA_OPT:
        //     break;
    }
    return new workStealingAlgorithm();
}

graph graphFactory(GraphType type, int shape)
{
    switch(type) {
    case GraphType::TORUS_2D:
        return torus2D(shape);
    case GraphType::TORUS_2D_60:
        return torus2D60(shape);
    case GraphType::TORUS_3D:
        return torus3D(shape);
    case GraphType::TORUS_3D_40:
        return torus3D40(shape);
    case GraphType::RANDOM:
    case GraphType::KGRAPH:
    default:
        return torus2D(shape);
    }

}

graph buildFromParents(std::atomic<int>* parents, int totalParents, int root, bool directed)
{
    graph g(directed, root, totalParents, GraphType::RANDOM);
    for (int i = 0; i < totalParents; i++) {
        g.addEdge(i, parents[i].load());
    }
    return g;
}


bool isCyclic(graph& g, std::unique_ptr<bool[]>& visited)
{
    int root = g.getRoot();
    std::unique_ptr<int[]> parents = std::make_unique<int[]>(g.getNumberVertices());
    std::fill(parents.get(), parents.get() + g.getNumberVertices(), BOTTOM);
    std::queue<int> q;
    visited[root] = true;
    q.push(root);
    int u;
    if (g.isDirected()) {
        while(!q.empty()) {
            u = q.front(); q.pop();
            std::list<int>& children = g.getChildren(u);
            for(auto const& v : children) {
                if (!visited[v]) {
                    visited[v] = true;
                    q.push(v);
                    parents[v] = u;
                } else if (parents[u] != v) {
                    return true;
                }
            }
        }
    } else {
        while (!q.empty()) {
            u = q.front(); q.pop();
            std::list<int>& neighbours = g.getNeighbours(u);
            for(auto const& v : neighbours) {
                if (!visited[v]) {
                    visited[v] = true;
                    q.push(v);
                    parents[v] = u;
                } else if (parents[u] != v) {
                    return true;
                }
            }
        }
    }
    return false;
}

bool hasCycle(graph& g)
{
    int numVertices = g.getNumberVertices();
    std::unique_ptr<bool[]> visited = std::make_unique<bool[]>(numVertices);
    std::fill(visited.get(), visited.get() + numVertices, false);
    bool cycle = isCyclic(g, visited);
    return cycle;
}

bool isTree(graph& g)
{
    int numVertices = g.getNumberVertices();
    std::unique_ptr<bool[]> visited = std::make_unique<bool[]>(numVertices);
    std::fill(visited.get(), visited.get() + numVertices, false);
    bool gHasCycle = isCyclic(g, visited);
    if (gHasCycle) {
        return false;
    }
    for (int u = 0; u < numVertices; u++) {
       if (u != g.getRoot() && !visited[u]) {
           return false;
       }
    }
    return true;
}

GraphCycleType detectCycleType(graph& g)
{
    int numVertices = g.getNumberVertices();
    std::unique_ptr<bool[]> visited = std::make_unique<bool[]>(numVertices);
    std::fill(visited.get(), visited.get() + numVertices, false);
    bool gHasCycle = isCyclic(g, visited);
    if (gHasCycle) {
        return GraphCycleType::CYCLE;
    }
    auto const check = [](bool x) { return x == true; };
    auto all = std::all_of(visited.get(), visited.get() + numVertices, check);
    if (!all) {
        return GraphCycleType::DISCONNECTED;
    }
    return GraphCycleType::TREE;
}

bool inArray(int val, int* array, int size) {
    return std::find(array, array + size, val) != array + size;
}

int* stubSpanning(graph& g, int size)
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distrib(1, g.getNumberVertices());
    int* stubSpanning = new int[size];
    int randomVal = distrib(gen);
    int i = 0;
    std::list<int> neighbors;
    std::deque<int> s;
    s.push_front(randomVal);
    int idx, tmpVal;
    while (i < size) {
        idx = s.front();
        s.pop_front();
        neighbors = g.getNeighbours(idx);
        for (auto it = neighbors.begin(); it != neighbors.end(); it++) {
            tmpVal = *it;
            if (std::find(s.begin(), s.end(), tmpVal) == s.end() &&
                !inArray(tmpVal, stubSpanning, size)) {
                s.push_front(tmpVal);
            }
        }
        if (!inArray(idx, stubSpanning, size)) {
            stubSpanning[i++] = idx;
        }
    }
    return stubSpanning;
};
