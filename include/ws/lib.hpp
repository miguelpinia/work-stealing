#pragma once

#include <list>
#include <vector>
#include <atomic>
#include <mutex>

int suma(int a, int b);


///////////////
// Constants //
///////////////


static const int EMPTY = -1;
static const int BOTTOM = -2;
static const int TOP = -3;


///////////
// Enums //
///////////


enum class GraphType {
    TORUS_2D,
    TORUS_2D_60,
    TORUS_3D,
    TORUS_3D_40,
    RANDOM,
    KGRAPH
};

enum class AlgorithmType {
    SIMPLE,  // No work-stealing algorithm
    CHASELEV, // Chase-Lev work-stealing algorithm
    CILK,  // Cilk THE work-stealing algorithm
    IDEMPOTENT_DEQUE, // Idempotent work-stealing doble queue
    IDEMPOTENT_LIFO,  // Idempotent work-stealing last-in first-out
    IDEMPOTENT_FIFO,  // Idempotent work-stealing first-in first-out
    WS_NC_MULT_OPT,   // Work-stealing with multiplicity optimized ("infinite array")
    WS_NC_MULT_LA_OPT,// Work-stealing with multiplicity optimized ("linked-lists")
    B_WS_NC_MULT_OPT, // Work-stealing bounded with multiplicity ("infinite array")
    B_WS_NC_MULT_LA_OPT // Work-stealing bounded with multiplicity ("linked-lists")
};

////////////////////////////
// Graphs and graph utils //
////////////////////////////


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
    graph(bool directed, int root, int numVertices, GraphType type);
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


//////////////////////////////
// Work-stealing algorithms //
//////////////////////////////

class taskArrayWithSize
{
private:
    int size;
    std::vector<int> array;
public:
    taskArrayWithSize();
    taskArrayWithSize(int size);
    taskArrayWithSize(int size, int defaultValue);
    int& getSize();
    int& get(int position);
    void set(int position, int value);
};

class workStealingAlgorithm
{
public:
    virtual bool isEmpty() {return false;}
    virtual bool isEmpty(int label) {(void) label; return false;}
    virtual bool put(int task) {(void) task; return false;}
    virtual bool put(int task, int label) {(void) task; (void) label; return false;}
    virtual int take() { return -1; }
    virtual int take(int label) { (void) label; return -1;}
    virtual int steal() { return -1;}
    virtual int steal(int label) {(void) label; return -1;}
};

class wsncmult : public workStealingAlgorithm
{
private:
    int tail;
    int size;
    std::atomic<int> Head;
    std::vector<int> head;
    std::vector<int> tasks;
public:
    wsncmult(int size, int numThreads);
    bool put(int task, int label);
    int take(int label);
    int steal(int label);
    bool isEmpty(int label);
    void expand();
};

class chaselev : public workStealingAlgorithm
{
private:
    std::atomic<int> H;
    std::atomic<int> T;
    std::atomic<int> tasksSize;
    std::atomic<int> *tasks;
public:
    chaselev(int initialSize);
    bool isEmpty();
    bool put(int task);
    int take();
    int steal();
    void expand();
    int getSize();
};

class cilk : public workStealingAlgorithm
{
private:
    std::atomic<int> H;
    std::atomic<int> T;
    std::atomic<int> tasksSize;
    std::atomic<int> *tasks;
    std::mutex mtx;
public:
    cilk(int initialSize);
    bool isEmpty();
    bool put(int task);
    int take();
    int steal();
    void expand();
    int getSize();
};

class idempotentFIFO : public workStealingAlgorithm
{
private:
    std::atomic<int> head;
    std::atomic<int> tail;
    taskArrayWithSize tasks;
public:
    idempotentFIFO(int size);
    bool isEmpty();
    bool put(int task);
    int take();
    int steal();
    void expand();
    int getSize();
};
