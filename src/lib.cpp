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

taskArrayWithSize::taskArrayWithSize(int size, int defaultValue) : size(size)
{
    array = std::vector<int>(size, defaultValue);
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

//////////////////////////////////////////
// Work stealing algorithms and classes //
//////////////////////////////////////////

wsncmult::wsncmult(int size, int numThreads) : tail(-1), size(size)
{
    Head = 0;
    head = std::vector<int>(numThreads, 0);
    tasks = std::vector<int>(size, BOTTOM);
}

bool wsncmult::isEmpty(int label)
{
    (void) label;
    return Head.load() > tail;
}

bool wsncmult::put(int task, int label)
{
    (void)label;
    tail++;
    if (tail == (int)tasks.size() - 1) expand();
    if (tail <= (int)tasks.size() - 3) {
        tasks[tail + 1] = BOTTOM;
        tasks[tail + 2] = BOTTOM;
    }
    tasks[tail] = task;
    return true;
}

int wsncmult::take(int label)
{
    head[label] = std::max(head[label], Head.load());
    if (head[label] <= tail) {
        int x = tasks[head[label]];
        head[label]++;
        Head.store(head[label]);
        return x;
    }
    return EMPTY;
}

int wsncmult::steal(int label)
{
    head[label] = std::max(head[label], Head.load());
    if (head[label] <= tail) {
        int x = tasks[head[label]];
        if (x != BOTTOM) {
            head[label]++;
            Head.store(head[label]);
            return x;
        }
    }
    return EMPTY;
}

void wsncmult::expand()
{}

///////////////////////////////////////
// Chase-Lev work-stealing algorithm //
///////////////////////////////////////

// We're following the description provided by Morrison and Afek from
// the article "Fence-Free Work Stealing on Bounded TSO Processors" to
// implement Chase-Lev work-stealing algorithm
chaselev::chaselev(int initialSize)
{
    H = 0;
    T = 0;
    tasksSize = initialSize;
    tasks = new std::atomic<int>[initialSize];
    for (int i = 0; i < initialSize; i++) {
        tasks[i] = 0;
    }
}

bool chaselev::isEmpty()
{
    int tail = T.load();
    int head = H.load();
    return head >= tail;
}

void chaselev::expand()
{
    int newSize = 2 * tasksSize.load();
    std::atomic<int> *newData = new std::atomic<int>[newSize];
    for (int i = 0; i < tasksSize; i++) {
        newData[i] = tasks[i].load();
    }
    for (int i = tasksSize; i < newSize; i++) {
        newData[i] = 0;
    }
    tasks = newData;
    tasksSize.store(newSize);
}

bool chaselev::put(int task)
{
    int tail = T.load();
    if (tail >= tasksSize) {
        expand();
        put(task);
    }
    tasks[tail % tasksSize] = task;
    T.store(tail + 1);
    return true;
}

int chaselev::take()
{
    int tail = T.load() - 1;
    T.store(tail);
    // In C++, the language doesn't have support for StoreLoad
    // fence. But using atomic thread fence with memory_order_seq_cst,
    // it's possible that compiler would add MFENCE fence.
    std::atomic_thread_fence(std::memory_order_seq_cst);
    int h = H.load();
    if (tail > h) {
        return tasks[tail % tasksSize];
    }
    if (tail < h) {
        T.store(h);
        return EMPTY;
    }
    T.store(h + 1);
    if (!H.compare_exchange_strong(h, h + 1)){
        return EMPTY;
    } else {
        return tasks[tail % tasksSize];
    }
}

int chaselev::steal()
{
    while(true) {
        int h = H.load();
        int t = T.load();
        if (h >= t) {
            return EMPTY;
        }
        int task = tasks[h % tasksSize];
        if (!H.compare_exchange_strong(h, h + 1)) continue;
        return task;
    }
}

int chaselev::getSize()
{
    return tasksSize;
}

//////////////////////////////////
// Cilk work-stealing algorithm //
//////////////////////////////////

cilk::cilk(int initialSize)
{
    H = 0;
    T = 0;
    tasksSize = initialSize;
    tasks = new std::atomic<int>[initialSize];
    for (int i = 0; i < initialSize; i++) {
        tasks[i] = 0;
    }
}

bool cilk::isEmpty()
{
    int tail = T.load();
    int head = H.load();
    return head >= tail;
}

void cilk::expand()
{
    int newSize = 2 * tasksSize.load();
    std::atomic<int> *newData = new std::atomic<int>[newSize];
    for (int i = 0; i < tasksSize; i++) {
        newData[i] = tasks[i].load();
    }
    for (int i = tasksSize; i < newSize; i++) {
        newData[i] = 0;
    }
    tasks = newData;
    tasksSize.store(newSize);
}

int cilk::getSize()
{
    return tasksSize;
}

bool cilk::put(int task)
{
    int tail = T.load();
    if (tail >= tasksSize) {
        expand();
        put(task);
    }
    tasks[tail % tasksSize] = task;
    T.store(tail + 1);
    return true;
}

int cilk::take()
{
    int tail = T.load() - 1;
    T.store(tail);
    std::atomic_thread_fence(std::memory_order_seq_cst);
    int head = H.load();
    if (tail >= head) {
        return tasks[tail % tasksSize];
    }
    if (tail < head) {
        mtx.lock();
        if (H.load() >= (tail + 1)) {
            T.store(tail + 1);
            mtx.unlock();
            return EMPTY;
        }
        mtx.unlock();
    }
    return tasks[tail % tasksSize];
}

int cilk::steal()
{
    mtx.lock();
    int h = H.load();
    H.store(h + 1);
    std::atomic_thread_fence(std::memory_order_seq_cst);
    int ret;
    if ((h + 1) <= T.load()) {
        ret = tasks[h % tasksSize];
    } else {
        H.store(h);
        ret = EMPTY;
    }
    mtx.unlock();
    return ret;
}

/////////////////////////////////////////////
// Idempotent FIFO Work-stealing algorithm //
/////////////////////////////////////////////

idempotentFIFO::idempotentFIFO(int size)
{
    head = 0;
    tail = 0;
    tasks = taskArrayWithSize(size);
}

bool idempotentFIFO::isEmpty()
{
    int h = head.load();
    int t = tail.load();
    return h == t;
}

bool idempotentFIFO::put(int task)
{
    int h = head.load();
    int t = tail.load();
    if (t == (h + tasks.getSize())) {
        expand();
        put(task);
    }
    tasks.set(t % tasks.getSize(), task);
    std::atomic_thread_fence(std::memory_order_release);
    tail.store(t + 1);
    return true;
}

int idempotentFIFO::take()
{
    int h = head.load();
    int t = tail.load();
    if (h == t) return EMPTY;
    int task = tasks.get(h % tasks.getSize());
    head.store(h + 1);
    return task;
}

int idempotentFIFO::steal()
{
    while(true) {
        int h = head.load();
        std::atomic_thread_fence(std::memory_order_acquire);
        int t = tail.load();
        if (h == t) return EMPTY;
        std::atomic_thread_fence(std::memory_order_acquire);
        taskArrayWithSize* a = &tasks;
        int task = a->get(h % a->getSize());
        std::atomic_thread_fence(std::memory_order_acquire);
        if (head.compare_exchange_strong(h, h + 1)) {
            return task;
        }
    }
}

void idempotentFIFO::expand()
{
    int size = tasks.getSize();
    taskArrayWithSize a(2 * size);
    std::atomic_thread_fence(std::memory_order_release);
    int h = head.load();
    int t = tail.load();
    for (int i = h; i < t; i++) {
        a.set(i % a.getSize(), tasks.get(i % tasks.getSize()));
    }
    tasks = a;
    std::atomic_thread_fence(std::memory_order_seq_cst);
}

int idempotentFIFO::getSize()
{
    return tasks.getSize();
}
