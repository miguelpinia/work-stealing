#pragma once
#ifndef _LIB_HPP_
#define _LIB_HPP_

#include <list>
#include <vector>
#include <atomic>
#include <mutex>
#include <climits>
#include <chrono>
#include <thread>
#include <barrier>
#include <iostream>
#include <cassert>
#include <memory>
#include <string>
#include <stdexcept>
#include "nlohmann/json.hpp"

using json = nlohmann::json;

int suma(int a, int b);

///////////////
// Constants //
///////////////


static const int EMPTY = -1;
static const int BOTTOM = -2;
static const int TOP = -3;

static constexpr std::memory_order relaxed = std::memory_order_relaxed;
static constexpr std::memory_order consume = std::memory_order_consume;
static constexpr std::memory_order acquire = std::memory_order_acquire;
static constexpr std::memory_order release = std::memory_order_release;
static constexpr std::memory_order seq_cst = std::memory_order_seq_cst;


///////////
// Enums //
///////////


enum GraphType {
    TORUS_2D,
    TORUS_2D_60,
    TORUS_3D,
    TORUS_3D_40,
    RANDOM,
    KGRAPH
};

enum AlgorithmType {
    // SIMPLE,  // No work-stealing algorithm
    CHASELEV, // Chase-Lev work-stealing algorithm
    CILK,  // Cilk THE work-stealing algorithm
    IDEMPOTENT_FIFO,  // Idempotent work-stealing first-in first-out
    IDEMPOTENT_LIFO,  // Idempotent work-stealing last-in first-out
    IDEMPOTENT_DEQUE, // Idempotent work-stealing doble queue
    WS_NC_MULT_OPT,   // Work-stealing with multiplicity optimized ("infinite array")
    // WS_NC_MULT_LA_OPT,// Work-stealing with multiplicity optimized ("linked-lists")
    B_WS_NC_MULT_OPT, // Work-stealing bounded with multiplicity ("infinite array")
    // B_WS_NC_MULT_LA_OPT // Work-stealing bounded with multiplicity ("linked-lists")
    LAST
};

enum StepSpanningTreeType {
    COUNTER,
    DOUBLE_COLLECT
};

enum GraphCycleType {
    CYCLE,
    DISCONNECTED,
    TREE
};

////////////////////////////
// Graphs and graph utils //
////////////////////////////


class vertex {
private:
    bool directed;
    int value;
    std::list<int> neighbours_;
    std::list<int> children_;
public:
    vertex();

    vertex(bool directed, int value);

    bool isDirected() const;

    int getValue() const;

    // I want to return a constant list that shouldn't be modified.
    void setNeighbours(std::list<int> &neighbours);

    std::list<int> &getNeighbours();

    void setChildren(std::list<int> &children);

    std::list<int> &getChildren();

    void addNeighbour(int neighbour);

    void deleteNeighbour(int neighbour);

    void addChild(int neighbour);

    void deleteChild(int neighbour);
};

class edge {
private:
    int src;
    int dest;
public:
    edge();

    edge(int src, int dest);

    int getSrc() const;

    int getDest() const;
};


class graph {
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

    void addEdge(int src, int dest);

    void addEdge(edge edge);

    void addEdges(std::vector<edge> edges);

    void deleteEdge(edge edge);

    bool hasEdge(edge edge);

    std::list<int> &getNeighbours(int vertex);

    std::list<int> &getChildren(int vertex);

    int getRoot() const;

    int getNumberVertices() const;

    int getNumberEdges() const;

    GraphType getType();

    void setDirected(bool directed);

    bool isDirected();
};

//////////////////////////////
// Work-stealing algorithms //
//////////////////////////////

class taskArrayWithSize {
public:
    enum class error_code{bad_size = -1, bad_index = -2};
private:
    int size;
    std::atomic<int>* array;
public:
    taskArrayWithSize();
    explicit taskArrayWithSize(int size);
    taskArrayWithSize(int size, int defaultValue);
    taskArrayWithSize(const taskArrayWithSize& other);
    taskArrayWithSize(taskArrayWithSize &&other);

    taskArrayWithSize& operator=( const taskArrayWithSize& other );
    taskArrayWithSize& operator=(taskArrayWithSize &&other);
    ~taskArrayWithSize();
    int &getSize();

    int get(int position);

    void set(int position, int value);
};

class workStealingAlgorithm {
public:
    virtual bool isEmpty() { return false; }

    virtual bool isEmpty(int label) {
        (void) label;
        return false;
    }

    virtual bool put(int task) {
        (void) task;
        return false;
    }

    virtual bool put(int task, int label) {
        (void) task;
        (void) label;
        return false;
    }

    virtual int take() { return -1; }

    virtual int take(int label) {
        (void) label;
        return -1;
    }

    virtual int steal() { return -1; }

    virtual int steal(int label) {
        (void) label;
        return -1;
    }
    virtual void printType() {
        std::cout << "Simple" << std::endl;
    }
};

class chaselev : public workStealingAlgorithm {
private:
    std::atomic<int> H;
    std::atomic<int> T;
    int tasksSize;
    std::unique_ptr<std::atomic<int>[]> tasks;
public:
    explicit chaselev(int initialSize);

    bool isEmpty() override;

    bool put(int task) override;

    int take() override;

    int steal() override;

    void expand();

    int getSize();
    void printType() {
        std::cout << "chase-lev" << std::endl;
    }

};

class cilk : public workStealingAlgorithm {
private:
    std::atomic<int> H;
    std::atomic<int> T;
    int tasksSize;
    std::unique_ptr<std::atomic<int>[]> tasks;
    std::mutex mtx;
public:
    explicit cilk(int initialSize);

    bool isEmpty() override;

    bool put(int task) override;

    int take() override;

    int steal() override;

    void expand();

    int getSize();
    void printType() override {
        std::cout << "cilk" << std::endl;
    }
};

class idempotentFIFO : public workStealingAlgorithm {
private:
    std::atomic<int> head;
    std::atomic<int> tail;
    taskArrayWithSize tasks;
public:
    explicit idempotentFIFO(int size);

    bool isEmpty() override;

    bool put(int task) override;

    int take() override;

    int steal() override;

    void expand();

    int getSize();

    void printType() {
        std::cout << "idempotent FIFO" << std::endl;
    }
};


struct pair {
    int t;
    int g;
};

class idempotentLIFO : public workStealingAlgorithm {
private:
    taskArrayWithSize tasks;
    int capacity;
    pair p = {0, 0};
    std::atomic_ref<pair> anchor{p};
public:
    explicit idempotentLIFO(int size);

    bool isEmpty() override;

    bool put(int task) override;

    int take() override;

    int steal() override;

    void expand();

    int getSize();
    void printType() {
        std::cout << "idempotent LIFO" << std::endl;
    }

};

struct triplet {
    int head;
    int size;
    int tag;
};

class idempotentDeque : public workStealingAlgorithm {
private:
    int capacity;
    taskArrayWithSize tasks;
    triplet* tp = new triplet{0,0,0};
    std::atomic<triplet*> anchor{tp};
public:
    explicit idempotentDeque(int size);

    bool isEmpty() override;

    bool put(int task) override;

    int take() override;

    int steal() override;

    void expand();

    int getSize();

    void printType() {
        std::cout << "idempotent DEQUE" << std::endl;
    }
};

class wsncmult : public workStealingAlgorithm {
private:
    int tail;
    int capacity;
    std::atomic<int> Head;
    std::unique_ptr<int[]> head;
    std::atomic<int>* tasks;
public:
    wsncmult(int size, int numThreads);
    ~wsncmult() {
        delete[] tasks;
    }

    bool put(int task, int label) override;

    int take(int label) override;

    int steal(int label) override;

    bool isEmpty(int label) override;

    void expand();

    int getCapacity() const;
    void printType() {
        std::cout << "WSNC_MULT" << std::endl;
    }
};

class wsncmultla : public workStealingAlgorithm {
private:
    int tail;
    int size;
    std::atomic<int> Head;
    std::atomic<int>* head;
    std::atomic<int>* tasks;
public:
    wsncmultla(int size, int numThreads);

    ~wsncmultla() {
        delete[] head;
        delete[] tasks;
    }

    bool put(int task, int label);

    int take(int label);

    int steal(int label);

    bool isEmpty(int label);

    void expand();

    void printType() {
        std::cout << "WCNC_MULT_LA" << std::endl;
    }
};

class bwsncmult : public workStealingAlgorithm {
private:
    int tail;
    int capacity;
    int *head;
    std::atomic<int> Head = 0;
    std::atomic<int> *tasks;
    std::atomic<bool> *B;
public:
    bwsncmult();

    bwsncmult(int capacity, int numThreads);

    ~bwsncmult() {
        delete[] tasks;
        delete[] B;
    }

    bool put(int task, int label);

    int take(int label);

    int steal(int label);

    bool isEmpty(int label);

    void expand();

    int getCapacity() const;
    void printType() {
        std::cout << "BWSNC_MULT" << std::endl;
    }

};

class bwsncmultla : public workStealingAlgorithm {
private:
    int tail;
    int size;
    std::atomic<int> Head;
    int *head;
    std::vector<int> tasks;
    // std::vector<bool> B;

public:
    bwsncmultla(int size, int numThreads);

    bool put(int task, int label);

    int take(int label);

    int steal(int label);

    bool isEmpty(int label);

    void expand();
    void printType() {
        std::cout << "BWSNC_MULT_LA" << std::endl;
    }
};

/////////////////////////
// Auxiliary functions //
/////////////////////////
namespace ws {
    struct Params {
        GraphType graphType;
        int shape;
        bool report;
        int numThreads;
        AlgorithmType algType;
        int structSize;
        int numIterExps;
        StepSpanningTreeType stepSpanningType;
        bool directed;
        bool stealTime;
        bool allTime;
        bool specialExecution;
    };

    void to_json(json& j, const Params& p);
    void from_json(const json& j, Params& p);
}


struct Report {
    std::atomic<int> takes = 0;
    std::atomic<int> puts = 0;
    std::atomic<int> steals = 0;
    std::atomic<long long> maxSteal = LLONG_MIN;
    std::atomic<long long> minSteal = LLONG_MAX;
    std::atomic<long long> avgSteal = 0;
    std::atomic<long long> avgIter = 0;
    long long executionTime; // Maybe it could be change by some type provided in chronno header
    int numProcessors_;
    int* processors_;
    Report(int numProcessors, int* processors) : numProcessors_(numProcessors), processors_(processors) {}
    ~Report() {
        delete[] processors_;
    }
    void incTakes() { ++takes; }
    void incPuts() { ++puts; }
    void incSteals() { ++steals; }

};

class AbstractStepSpanningTree
{
public:
    int root_;
    int label_;
    int numThreads_;
    bool stealTime_;
    graph& g_;
    Report& report_;
    std::atomic<int>* colors_;
    std::atomic<int>* parents_;
    workStealingAlgorithm* algorithm_;
    workStealingAlgorithm** algorithms_;


    AbstractStepSpanningTree(int root, int label, bool stealTime,
                             graph& g, std::atomic<int>* colors,
                             std::atomic<int>* parents,
                             workStealingAlgorithm* algorithm,
                             workStealingAlgorithm** algorithms,
                             Report& report, int numThreads)
        : root_(root),
          label_(label),
          numThreads_(numThreads),
          stealTime_(stealTime),
          g_(g),
          report_(report),
          colors_(colors),
          parents_(parents),
          algorithm_(algorithm),
          algorithms_(algorithms)  {}


    virtual void graph_traversal_step() = 0;
};

template<typename ... Args>
std::string string_format( const std::string& format, Args ... args );

template<typename ... Args>
std::string string_format( const std::string& format, Args ... args )
{
    int size_s = std::snprintf( nullptr, 0, format.c_str(), args ... ) + 1; // Extra space for '\0'
    if( size_s <= 0 ){ throw std::runtime_error( "Error during formatting." ); }
    auto size = static_cast<size_t>( size_s );
    std::unique_ptr<char[]> buf( new char[ size ] );
    std::snprintf( buf.get(), size, format.c_str(), args ... );
    return std::string( buf.get(), buf.get() + size - 1 ); // We don't want the '\0' inside
}

int pickRandomThread(int numThreads, int processor);

class CounterStepSpanningTree : public AbstractStepSpanningTree
{
private:
    bool specialExecution_;
    std::atomic<int>& counter_;
    std::atomic<int>* visited_;

public:
    CounterStepSpanningTree(int root, int label, bool stealTime,
                            graph& g, std::atomic<int>* colors,
                            std::atomic<int>* parents,
                            workStealingAlgorithm* algorithm,
                            workStealingAlgorithm* algorithms[],
                            Report& report, int numThreads,
                            bool specialExecution,
                            std::atomic<int>& counter,
                            std::atomic<int>* visited)
    : AbstractStepSpanningTree(root, label, stealTime, g, colors, parents,
                               algorithm, algorithms, report, numThreads),
      specialExecution_(specialExecution), counter_(counter),
      visited_(visited)
    {}

    void graph_traversal_step();
    void generalExecution();
    void specialExecution();

};


int mod(int a, int b);
graph torus2D(int shape);
graph directedTorus2D(int shape);
graph torus2D60(int shape);
graph directedTorus2D60(int shape);
graph torus3D(int shape);
graph directedTorus3D(int shape);
graph torus3D40(int shape);
graph directedTorus3D40(int shape);
// graph random(int numberVertices, int vertexDegree);
// graph directedRandom(int numberVertices, int vertexDegree);
graph buildFromParents(std::atomic<int>* parents, int totalParents, int root, bool directed);

bool isCyclic(graph& g, std::unique_ptr<bool[]>& visited);
bool hasCycle(graph* g);
bool isTree(graph& g);

graph spanningTree(graph& g, int* roots, Report& report, ws::Params& params);

GraphCycleType detectCycleType(graph& g);

workStealingAlgorithm* workStealingAlgorithmFactory(AlgorithmType algType, int capacity, int numThreads);

int* stubSpanning(graph& g, int size);

bool inArray(int val, int array[], int size);

json compare(json properties);

graph graphFactory(GraphType, int shape, bool directed);

json experiment(ws::Params &params, graph &g);

json experimentComplete(GraphType type, int shape, bool directed);

std::unordered_map<AlgorithmType, std::vector<json>> buildLists();

std::string getAlgorithmTypeFromEnum(AlgorithmType type);


// class MemManager {
//     void register_thread(int num); // Called once, before any call to op_begin(), num indicate the maximum number of locations the caller can reserve
//     void unregister_thread(); // Called once, after the last call to op_end()
//     void op_begin(); // Indicate the beginning of a concurrent operation
//     void op_end(); // indicate the end of a concurrent operation
//     bool try_reserve(void* ptr); // try to protect a pointer from reclamation
//     void unreserve(void* ptr); // stop protecting a pointer
//     void sched_for_reclaim(void* ptr); // try to reclam a pointer
// }

#endif /* _LIB_HPP_ */
