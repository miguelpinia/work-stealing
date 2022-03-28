#include "ws/lib.hpp"
#include <algorithm>
#include <iostream>
#include <stack>
#include <deque>
#include <unordered_map>
#include <list>
#include <fstream>
#include <cassert>
#include <memory>
#include <string>
#include <stdexcept>

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

int suma(int a, int b) {
    return a + b;
}

graph spanningTree(graph& g, int* roots, Report& report, ws::Params& params)
{
    std::vector<std::thread> threads;
    std::atomic<int>* colors = new std::atomic<int>[g.getNumberVertices()];
    std::atomic<int>* parents = new std::atomic<int>[g.getNumberVertices()];
    std::atomic<int>* visited = new std::atomic<int>[g.getNumberVertices()];
    for (int i = 0; i < g.getNumberVertices(); i++) {
        colors[i] = 0; parents[i] = -1; visited[i] = 0;
    }

    workStealingAlgorithm* algs[params.numThreads];
    int* processors = new int[params.numThreads];
    std::atomic<int> counter = 0;
    auto t_start = std::chrono::high_resolution_clock::now();
    auto wait_for_begin = []() noexcept {};
    for(int i = 0; i < params.numThreads; i++) {
        workStealingAlgorithm* c = workStealingAlgorithmFactory(params.algType,
                                                                params.structSize,
                                                                params.numThreads);
        algs[i] = c;
    }
    std::barrier sync_point(params.numThreads, wait_for_begin);
    std::cout << getAlgorithmTypeFromEnum(params.algType) << std::endl;
    for (int i = 0; i < params.numThreads; i++) {
        std::function<void(int)> func = [&](int processID) {
            workStealingAlgorithm* alg = algs[processID];
            CounterStepSpanningTree step(roots[processID], (processID + 1), false,
                                         g, colors, parents, alg, algs, report,
                                         params.numThreads, params.specialExecution,
                                         counter, visited);
            sync_point.arrive_and_wait();
            step.graph_traversal_step();
        };
        threads.emplace_back(std::thread(func, i));
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(i, &cpuset);
        int rc = pthread_setaffinity_np(threads[i].native_handle(),
                                        sizeof(cpu_set_t), &cpuset);
        if (rc != 0) {
            std::cerr << "Error calling pthread_setaffinity_np: " << rc << "\n";
        }

    }
    for (std::thread &th : threads) {
        if (th.joinable()) {
            th.join();
        }
    }
    auto t_end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration<long, std::nano>(t_end-t_start).count();
    report.executionTime = duration;
    for (int i = 0; i < g.getNumberVertices(); i++) {
        if (colors[i].load() != 0) {
            processors[colors[i].load() - 1]++; // because i == 0 doesnt have parent.
        }
    }
    report.processors_ = processors;
    parents[roots[0]] = -1; // ensures that first node is always the root of the tree
    for (int i = 1; i < params.numThreads; i++) {
        parents[roots[i]].store(roots[i - 1]);
    }
    graph newGraph = buildFromParents(parents, g.getNumberVertices(), roots[0], g.isDirected());
    delete[] colors;
    delete[] parents;
    delete[] visited;
    return newGraph;
}

void CounterStepSpanningTree::graph_traversal_step()
{
    if (specialExecution_) {
        specialExecution();
    } else {
        generalExecution();
    }
}

void CounterStepSpanningTree::generalExecution()
{
    colors_[root_].store(label_);
    algorithm_->put(root_);
    // algorithm_->printType();
    int x = visited_[root_].exchange(1);
    if (x == 0) {
        counter_++;
    }
    report_.incPuts();
    int v, w, stolenItem, thread;
    do {
        while (!algorithm_->isEmpty()) {
            v = algorithm_->take();
            report_.incTakes();
            if (v >= 0) {
                std::list<int>& neighbors = g_.getNeighbours(v);
                for(auto it = neighbors.begin(); it != neighbors.end(); it++) {
                    w = *it;
                    if (colors_[w].load() == 0) {
                        colors_[w].store(label_);
                        parents_[w].store(v);
                        algorithm_->put(w);
                        x = visited_[w].exchange(1);
                        if (x == 0) {
                            counter_++;
                        }
                        report_.incPuts();
                    }
                }
            }
        }
        if (numThreads_ > 1) {
            thread = pickRandomThread(numThreads_, label_ - 1);
            stolenItem = algorithms_[thread]->steal();
            report_.incSteals();
            if (stolenItem >= 0) {
                algorithm_->put(stolenItem);
                report_.incPuts();
            }
        }
    } while(counter_.load() < g_.getNumberVertices());

}


void CounterStepSpanningTree::specialExecution()
{
    // label_ is ProcessID + 1, so, to use the work-stealing
    // algorithms with label, is necessary call the methods with
    // label_ - 1
    colors_[root_].store(label_);
    algorithm_->put(root_, label_ - 1);
    int x = visited_[root_].exchange(1);
    if (x == 0) {
        counter_++;
    }
    report_.incPuts();
    int v, w, stolenItem, thread;
    do {
        while (!algorithm_->isEmpty(label_ - 1)) {
            v = algorithm_->take(label_ - 1);
            report_.incTakes();
            if (v >= 0) {
                std::list<int>& neighbors = g_.getNeighbours(v);
                for(auto it = neighbors.begin(); it != neighbors.end(); it++) {
                    w = *it;
                    if (colors_[w].load() == 0) {
                        colors_[w].store(label_);
                        parents_[w].store(v);
                        algorithm_->put(w, label_ - 1);
                        x = visited_[w].exchange(1);
                        if (x == 0) {
                            counter_++;
                        }
                        report_.incPuts();
                    }
                }
            }
        }
        if (numThreads_ > 1) {
            thread = pickRandomThread(numThreads_, label_ -1);
            stolenItem = algorithms_[thread]->steal(label_ - 1);
            report_.incSteals();
            if (stolenItem >= 0) {
                algorithm_->put(stolenItem, label_ - 1);
                report_.incPuts();
            }
        }
    } while(counter_.load() < g_.getNumberVertices());
}

GraphType getGraphTypeFromString(std::string type) {
    if (type == "TORUS_2D") return GraphType::TORUS_2D;
    if (type == "TORUS_2D_60") return GraphType::TORUS_2D_60;
    if (type == "TORUS_3D") return GraphType::TORUS_3D;
    if (type == "TORUS_3D_40") return GraphType::TORUS_3D_40;
    return GraphType::RANDOM;
}

std::string getGraphTypeFromEnum(GraphType type) {
    if (type == GraphType::TORUS_2D) return "TORUS_2D";
    if (type == GraphType::TORUS_2D_60) return "TORUS_2D_60";
    if (type == GraphType::TORUS_3D) return "TORUS_3D";
    if (type == GraphType::TORUS_3D_40) return "TORUS_3D_40";
    return "RANDOM";
}

std::string getAlgorithmTypeFromEnum(AlgorithmType type)
{
    switch(type) {
    case AlgorithmType::CHASELEV:
        return "CHASELEV";
    case AlgorithmType::CILK:
        return "CILK";
    case AlgorithmType::IDEMPOTENT_DEQUE:
        return "IDEMPOTENT_DEQUE";
    case AlgorithmType::IDEMPOTENT_LIFO:
        return "IDEMPOTENT_LIFO";
    case AlgorithmType::IDEMPOTENT_FIFO:
        return "IDEMPOTENT_FIFO";
    case AlgorithmType::WS_NC_MULT_OPT:
        return "WS_NC_MULT";
    case AlgorithmType::B_WS_NC_MULT_OPT:
        return "B_WS_NC_MULT";
    // case AlgorithmType::B_WS_NC_MULT_LA_OPT:
    // case AlgorithmType::WS_NC_MULT_LA_OPT:
    // case AlgorithmType::SIMPLE:
    default:
        return "UNKNOWN";
    }
    return "UNKNOWN";
}

json experiment(ws::Params &params)
{
    graph g = graphFactory(params.graphType, params.shape);
    int* processors = new int[params.numThreads];
    Report r{params.numThreads, processors};
    int* roots = stubSpanning(g, params.numThreads);
    graph tree = spanningTree(g, roots, r, params);
    assert(isTree(tree));
    json result;
    result["numThreads"] = params.numThreads;
    result["executionTime"] = r.executionTime;
    result["takes"] = r.takes.load();
    result["puts"] = r.puts.load();
    result["steals"] = r.steals.load();
    result["graphType"] = getGraphTypeFromEnum(params.graphType);
    result["algorithm"] = getAlgorithmTypeFromEnum(params.algType);
    json par = params;
    return result;
}

int calculateStructSize(GraphType type, int shape) {
    switch(type) {
    case GraphType::TORUS_2D:
    case GraphType::TORUS_2D_60:
        return shape * shape;
    case GraphType::TORUS_3D:
    case GraphType::TORUS_3D_40:
        return shape * shape * shape;
    case GraphType::RANDOM:
    case GraphType::KGRAPH:
    default:
        return shape;
    }

}

bool isEspecial(AlgorithmType type)
{
    switch(type) {
    case AlgorithmType::WS_NC_MULT_OPT:
    // case AlgorithmType::WS_NC_MULT_LA_OPT:
    case AlgorithmType::B_WS_NC_MULT_OPT:
    // case AlgorithmType::B_WS_NC_MULT_LA_OPT:
        return true;
    default:
        return false;
    }
}

json experimentComplete(GraphType type, int shape)
{
    const int numProcessors = std::thread::hardware_concurrency();
    json last;
    std::unordered_map<AlgorithmType, std::vector<json>> data = buildLists();
    std::vector<json> values;
    for (int i = 0; i < numProcessors; i++) {
        int structSize = calculateStructSize(type, shape);
        for (int at = AlgorithmType::CHASELEV; at != AlgorithmType::LAST; at++) {
            AlgorithmType atype = static_cast<AlgorithmType>(at);
            bool special = isEspecial(atype);
            ws::Params p{type, shape, false,
                (i+ 1), atype, structSize, 10, StepSpanningTreeType::COUNTER,
                false, false, false, special};
            json result = experiment(p);
            data[atype].emplace_back(result);
            values.emplace_back(result);
        }
    }
    // std::cout << std::setw(4) << values << std::endl;
    last["values"] = values;
    return last;
}

json compare(json properties) {
    json result;
    int vertexSize = properties["VERTEX_SIZE"]; // Graph's shape
    int structSize = properties["STRUCT_SIZE"]; // Initial size for the task array of the work-stealing algorithm.
    // GraphType graphType = getGraphTypeFromString(properties["GRAPH_TYPE"]); // Graph type
    // bool directed = properties["DIRECTED"]; // Is directed the graph?
    // bool stealTime = properties["STEAL_TIME"]; // Should we take the time performed by steals?
    // int iterations = properties["ITERATIONS"]; // Number of iterations for experimentsn
    const auto processorNum = std::thread::hardware_concurrency(); // Number of processes in the system.
    // bool allTime = properties["ALL_TIME"];
    // Params params{graphType, vertexSize, false, }
    // std::unordered_map<AlgorithmType, std::list<Result>> lists = buildLists();
    std::cout << string_format("%d, %d, processors: %d\n", vertexSize, structSize, processorNum);
    result["FOO"] = 0;
    std::cout << result << std::endl;
    return result;
}

std::unordered_map<AlgorithmType, std::vector<json>> buildLists()
{
    std::unordered_map<AlgorithmType, std::vector<json>> lists;
    for (int at = AlgorithmType::CHASELEV; at != AlgorithmType::LAST; at++) {
        AlgorithmType atype = static_cast<AlgorithmType>(at);
        lists[atype] = std::vector<json>();
    }
    return lists;
}
