#include "ws/lib.hpp"
#include <algorithm>
#include <iostream>
#include <random>
#include <queue>
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

//////////////////////////
// Task array with size //
//////////////////////////

taskArrayWithSize::taskArrayWithSize() = default;
taskArrayWithSize::taskArrayWithSize(int size) : size(size) {
    array = std::make_unique<int[]>(size);
    std::fill(array.get(), array.get() + size, -1);
}

taskArrayWithSize::taskArrayWithSize(int size, int defaultValue) : size(size) {
    array = std::make_unique<int[]>(size);
    std::fill(array.get(), array.get() + size, defaultValue);
}

taskArrayWithSize::taskArrayWithSize(const taskArrayWithSize& other) : size(other.size), array(std::make_unique<int[]>(other.size)) {
    for (int i = 0; i < other.size; i++) array[i] = other.array[i];
}


int &taskArrayWithSize::getSize() {
    return size;
}

int &taskArrayWithSize::get(int position) {
    return array[position];
}

void taskArrayWithSize::set(int position, int value) {
    array[position] = value;
}

///////////////////////////////////////
// Chase-Lev work-stealing algorithm //
///////////////////////////////////////

// We're following the description provided by Morrison and Afek from
// the article "Fence-Free Work Stealing on Bounded TSO Processors" to
// implement Chase-Lev work-stealing algorithm
chaselev::chaselev(int initialSize) {
    H = 0;
    T = 0;
    tasksSize = initialSize;
    tasks = new std::atomic<int>[initialSize];
    for (int i = 0; i < initialSize; i++) {
        tasks[i] = 0;
    }
}

bool chaselev::isEmpty() {
    int tail = T.load();
    int head = H.load();
    return head >= tail;
}

void chaselev::expand() {
    int newSize = 2 * tasksSize.load();
    auto *newData = new std::atomic<int>[newSize];
    for (int i = 0; i < tasksSize; i++) {
        newData[i] = tasks[i].load();
    }
    for (int i = tasksSize; i < newSize; i++) {
        newData[i] = 0;
    }
    std::atomic<int> *tmp = tasks;
    tasks = newData;
    delete[] tmp;
    tasksSize.store(newSize);
}

bool chaselev::put(int task) {
    int tail = T.load();
    if (tail >= tasksSize) expand();
    tasks[tail % tasksSize] = task;
    T.store(tail + 1);
    return true;
}

int chaselev::take() {
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
    if (!H.compare_exchange_strong(h, h + 1)) {
        return EMPTY;
    } else {
        return tasks[tail % tasksSize];
    }
}

int chaselev::steal() {
    while (true) {
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

int chaselev::getSize() {
    return tasksSize;
}

//////////////////////////////////
// Cilk work-stealing algorithm //
//////////////////////////////////

cilk::cilk(int initialSize) {
    H = 0;
    T = 0;
    tasksSize = initialSize;
    tasks = new std::atomic<int>[initialSize];
    for (int i = 0; i < initialSize; i++) {
        tasks[i] = 0;
    }
}

bool cilk::isEmpty() {
    int tail = T.load();
    int head = H.load();
    return head >= tail;
}

void cilk::expand() {
    int newSize = 2 * tasksSize.load();
    auto *newData = new std::atomic<int>[newSize];
    for (int i = 0; i < tasksSize; i++) {
        newData[i] = tasks[i].load();
    }
    for (int i = tasksSize; i < newSize; i++) {
        newData[i] = 0;
    }
    std::atomic<int> *tmp = tasks;
    tasks = newData;
    delete[] tmp;
    tasksSize.store(newSize);
}

int cilk::getSize() {
    return tasksSize;
}

bool cilk::put(int task) {
    int tail = T.load();
    if (tail >= tasksSize) expand();
    tasks[tail % tasksSize] = task;
    T.store(tail + 1);
    return true;
}

int cilk::take() {
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

int cilk::steal() {
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

idempotentFIFO::idempotentFIFO(int size) {
    head = 0;
    tail = 0;
    tasks = taskArrayWithSize(size);
}

bool idempotentFIFO::isEmpty() {
    int h = head.load();
    int t = tail.load();
    return h == t;
}

bool idempotentFIFO::put(int task) {
    int h = head.load();
    int t = tail.load();
    if (t == (h + tasks.getSize())) expand();
    tasks.set(t % tasks.getSize(), task);
    std::atomic_thread_fence(std::memory_order_release);
    tail.store(t + 1);
    return true;
}

int idempotentFIFO::take() {
    int h = head.load();
    int t = tail.load();
    if (h == t) return EMPTY;
    int task = tasks.get(h % tasks.getSize());
    head.store(h + 1);
    return task;
}

int idempotentFIFO::steal() {
    while (true) {
        int h = head.load();
        std::atomic_thread_fence(std::memory_order_acquire);
        int t = tail.load();
        if (h == t) return EMPTY;
        std::atomic_thread_fence(std::memory_order_acquire);
        taskArrayWithSize *a = &tasks;
        int task = a->get(h % a->getSize());

        std::atomic_thread_fence(std::memory_order_acquire);
        if (head.compare_exchange_strong(h, h + 1)) {
            return task;
        }
    }
}

void idempotentFIFO::expand() {
    int size = tasks.getSize();
    taskArrayWithSize a(2 * size);
    std::atomic_thread_fence(std::memory_order_release);
    int h = head.load();
    int t = tail.load();
    for (int i = h; i < t; i++) {
        a.set(i % a.getSize(), tasks.get(i % tasks.getSize()));
    }
    tasks = std::move(a);
    std::atomic_thread_fence(std::memory_order_seq_cst);
}

int idempotentFIFO::getSize() {
    return tasks.getSize();
}


/////////////////////////////////////////////
// Idempotent LIFO Work-Stealing algorithm //
/////////////////////////////////////////////

// pair::pair() : t(0), g(0) {}
// pair::pair(int t, int g) : t(t), g(g) {}

idempotentLIFO::idempotentLIFO(int size) {
    tasks = std::make_unique<int[]>(size);
    capacity = size;
    pair *p = new pair(0, 0);
    anchor.store(p);
    std::fill(tasks.get(), tasks.get() + size, BOTTOM);
}

bool idempotentLIFO::isEmpty() {
    return anchor.load()->t == 0;
}

bool idempotentLIFO::put(int task) {
    auto[t, g] = *anchor.load();
    if (t == capacity) expand();
    tasks[t] = task;
    std::atomic_thread_fence(std::memory_order_release);
    anchor.store(new pair(t + 1, g + 1));
    return true;
}

int idempotentLIFO::take() {
    auto[t, g] = *anchor.load();
    if (t == 0) return EMPTY;
    int task = tasks[t - 1];
    anchor.store(new pair(t - 1, g));
    return task;
}

int idempotentLIFO::steal() {
    while (true) {
        pair *oldReference = anchor.load();
        auto[t, g] = *oldReference;
        if (t == 0) return EMPTY;
        int *tmp = tasks.get();
        std::atomic_thread_fence(std::memory_order_release);
        int task = tmp[t - 1];
        pair *newPair = new pair(t - 1, g);
        if (anchor.compare_exchange_strong(oldReference, newPair)) {
            delete oldReference;
            return task;
        }
    }
}

void idempotentLIFO::expand() {
    int *newTasks = new int[2 * capacity];
    for (int i = 0; i < capacity; i++) {
        newTasks[i] = tasks[i];
    }
    tasks.reset(newTasks);
    std::atomic_thread_fence(std::memory_order_release);
    capacity = 2 * capacity;
    std::atomic_thread_fence(std::memory_order_release);
}

int idempotentLIFO::getSize() const {
    return capacity;
}

//////////////////////////////////////////////
// Idempotent DEQUE work-stealing algorithm //
//////////////////////////////////////////////

triplet::triplet(int _head, int _size, int _tag) {
    this->head = _head;
    this->size = _size;
    this->tag = _tag;
}

idempotentDeque::idempotentDeque(int size) {
    capacity = size;
    tasks = taskArrayWithSize(size);
    auto *t = new triplet(0, 0, 0);
    anchor.store(t);
}

bool idempotentDeque::isEmpty() {
    return anchor.load()->size == 0;
}

bool idempotentDeque::put(int task) {
    auto[head, size, tag] = *anchor.load();
    if (size == tasks.getSize()) {
        expand();
        return put(task);
    }
    tasks.set((head + size) % tasks.getSize(), task);
    std::atomic_thread_fence(std::memory_order_release);
    anchor.store(new triplet(head, size + 1, tag + 1));
    return true;
}

int idempotentDeque::take() {
    auto [head, size, tag] = *anchor.load();
    if (size == 0) return EMPTY;
    int task = tasks.get((head + size - 1) % tasks.getSize());
    anchor.store(new triplet(head, size - 1, tag));
    return task;
}

int idempotentDeque::steal() {
    while (true) {
        triplet* oldReference = anchor.load();
        auto[head, size, tag] = *oldReference;
        if (size == 0) return EMPTY;
        std::atomic_thread_fence(std::memory_order_acquire);
        taskArrayWithSize* a = &tasks;
        auto task = a->get(head % a->getSize());
        auto h2 = (head + 1) % a->getSize();
        std::atomic_thread_fence(std::memory_order_acquire);
        if (anchor.compare_exchange_strong(oldReference, new triplet(h2, size - 1, tag))) {
            delete oldReference;
            return task;
        }
    }
}

void idempotentDeque::expand() {
    triplet *old = anchor.load();
    auto[head, size, tag] = *old;
    taskArrayWithSize a(2 * tasks.getSize());
    for (int i = 0; i < size; i++) {
        a.set((head + i) % a.getSize(), tasks.get((head + i) % tasks.getSize()));
    }
    std::atomic_thread_fence(std::memory_order_release);
    tasks = a;
    std::atomic_thread_fence(std::memory_order_release);
}

int idempotentDeque::getSize() {
    return tasks.getSize();
}

//////////////////////////////////////////
// Work stealing algorithms and classes //
//////////////////////////////////////////

wsncmult::wsncmult(int capacity, int numThreads) : tail(-1), capacity(capacity) {
    Head = 0;
    head = std::make_unique<int[]>(numThreads);
    std::fill(head.get(), head.get() + numThreads, 0);
    tasks = new std::atomic<int>[capacity];
    std::fill(tasks, tasks + capacity, BOTTOM);
}

bool wsncmult::isEmpty(int label) {
    return head[label] > tail;
}

bool wsncmult::put(int task, int label) {
    (void) label;
    if (tail == capacity - 1) expand();
    if (tail <= capacity - 3) {
        tasks[tail + 1] = BOTTOM;
        tasks[tail + 2] = BOTTOM;
    }
    tail++;
    tasks[tail] = task;
    return true;
}

int wsncmult::take(int label) {
    head[label] = std::max(head[label], Head.load());
    if (head[label] <= tail) {
        int x = tasks[head[label]];
        head[label]++;
        Head.store(head[label]);
        return x;
    }
    return EMPTY;
}

int wsncmult::steal(int label) {
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

void wsncmult::expand() {
    auto newCapacity = 2 * capacity;
    auto newData = new std::atomic<int>[newCapacity];
    for (int i = 0; i < capacity; i++) newData[i] = tasks[i].load();
    tasks = newData;
    std::atomic_thread_fence(std::memory_order_release);
    capacity = 2 * capacity;
    std::atomic_thread_fence(std::memory_order_release);
}

int wsncmult::getCapacity() const {
    return capacity;
}


////////////////////////////////////////////////////
// Bounded work-stealing algorithm implementation //
////////////////////////////////////////////////////

bwsncmult::bwsncmult() : tail(-1), capacity(0) {}

bwsncmult::bwsncmult(int capacity, int numThreads) : tail(-1), capacity(capacity)
{
    head = new int[numThreads];
    tasks = new std::atomic<int>[capacity];
    B = new std::atomic<bool>[capacity];
    for (int i = 0; i < numThreads; i++) {
        head[i] = 0;
    }
    for (int i = 0; i < capacity; i++) {
        tasks[i] = 0;
        B[i] = false;
    }
    B[0] = true;
    B[1] = true;
}

bool bwsncmult::isEmpty(int label)
{
    (void) label;
    return Head.load() > tail;
}

bool bwsncmult::put(int task, int label)
{
    (void) label;
    tail++;
    if (tail == capacity) expand();
    if (tail <= capacity - 3) {
        tasks[tail + 1] = BOTTOM;
        tasks[tail + 2] = BOTTOM;
        B[tail + 1] = true;
        B[tail + 2] = true;
    }
    tasks[tail] = task;
    return true;
}

int bwsncmult::take(int label)
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

int bwsncmult::steal(int label)
{
    while (true) {
        head[label] = std::max(head[label], Head.load());
        if (head[label] <= tail) {
            int x = tasks[head[label]];
            if (x != BOTTOM) {
                int h = head[label];
                head[label]++;
                if (B[h].exchange(false)) {
                    Head.store(head[label]);
                    return x;
                }
            } else {
                return EMPTY;
            }
        } else {
            return EMPTY;
        }
    }
}

int bwsncmult::getCapacity() const {
    return capacity;
}

void bwsncmult::expand() {
    auto newCapacity = 2 * capacity;
    auto newData = new std::atomic<int>[newCapacity];
    for (int i = 0; i < newCapacity; i++) {
        newData[i] = BOTTOM;
    }

    std::atomic<bool>* newState = new std::atomic<bool>[newCapacity];
    for (int i = 0; i < newCapacity; i++) newState[i] = true;
    std::atomic_thread_fence(std::memory_order_release);

    for (int i = 0; i < capacity; i++) {
        newData[i] = tasks[i].load();
        newState[i] = B[i].load();
    }
    std::atomic<int> *tmp1 = tasks;
    std::atomic<bool> *tmp2 = B;
    tasks = newData;
    std::atomic_thread_fence(std::memory_order_release);
    B = newState;
    std::atomic_thread_fence(std::memory_order_release);
    delete[] tmp1;
    delete[] tmp2;
    capacity = 2 * capacity;
    std::atomic_thread_fence(std::memory_order_release);
}

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
        j = (k / 4) % shape;
        i = k / (shape * 4);
        currentIdx = (i * shape) + j;
        pos = k % 4;
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

graph torus2D_60(int shape)
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distrib(1, shape);
    bool directed = false;
    int numVertices = shape * shape;
    int numEdges = numVertices * 4;
    graph g(directed, 0, numVertices, GraphType::TORUS_2D_60);
    int randomNumber, i, j, currentIdx, pos;
    int neighbor;
    for (int k = 0; k < numEdges; k++) {
        i = k / (shape * 4);
        j = (k / 4) % shape;
        currentIdx = (i * shape) + j;
        pos = k % 4;
        randomNumber = distrib(gen);
        switch(pos) {
        case 0:
            neighbor = mod((i - 1), shape) * shape + j;
            g.addEdge(currentIdx, neighbor);
            break;
        case 1:
            if (randomNumber <= 60) {
                neighbor = i * shape + mod((j + 1), shape);
                g.addEdge(currentIdx, neighbor);
            }
            break;
        case 2:
            if (randomNumber <= 60) {
                neighbor = mod((i + 1), shape) * shape + j;
                g.addEdge(currentIdx, neighbor);
            }
            break;
        case 3:
            if (randomNumber <= 60) {
                neighbor = i * shape + mod((j - 1), shape);
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
        k = (m / 6) % shape;
        j = (m / (shape * 6)) % shape;
        i = (m / (shape * shape * 6)) % shape;
        currentIdx = (i * shape * shape) + (j * shape) + k;
        pos = m % 6;
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

graph torus3D40(int shape)
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distrib(1, shape);
    bool directed = false;
    int numVertices = shape * shape * shape;
    int numEdges = numVertices * 6;
    graph g(directed, 0, numVertices, GraphType::TORUS_3D_40);
    int i, j, k, currentIdx, pos, neighbor, randomNum;
    for (int m = 0; m < numEdges; m++) {
        k = (m/6) % shape;
        j = (m / (shape * 6)) % shape;
        i = (m / (shape * shape * 6)) % shape;
        currentIdx = (i * shape * shape) + (j * shape) + k;
        pos = m % 6;
        randomNum = distrib(gen);
        switch (pos) {
        case 0:
            neighbor = (i * shape * shape) + (j * shape) + mod(k - 1, shape);
            g.addEdge(currentIdx, neighbor);
            break;
        case 1:
            if (randomNum <= 40) {
                neighbor = (i * shape * shape) + (j * shape) + mod(k + 1, shape);
                g.addEdge(currentIdx, neighbor);
            }
            break;
        case 2:
            if (randomNum <= 40) {
                neighbor = (i * shape * shape) + mod(j - 1, shape) * shape + k;
                g.addEdge(currentIdx, neighbor);
            }
            break;
        case 3:
            if (randomNum <= 40) {
                neighbor = (i * shape * shape) + mod(j + 1, shape) * shape + k;
                g.addEdge(currentIdx, neighbor);
            }
            break;
        case 4:
            if (randomNum <= 40) {
                neighbor = (mod(i - 1, shape) * shape * shape) + (j * shape) + k;
                g.addEdge(currentIdx, neighbor);
            }
            break;
        case 5:
            if (randomNum <= 40) {
                neighbor = (mod(i + 1, shape) * shape * shape) + (j * shape) + k;
            }
            break;
        default:
            break;
        }
    }
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
        return torus2D_60(shape);
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

bool isCyclic(graph& g, bool* visited)
{
    int root = g.getRoot();
    int* parents = new int[g.getNumberVertices()];
    std::queue<int> q;
    visited[root] = true;
    q.push(root);
    int u;
    if (g.isDirected()) {
        while(!q.empty()) {
            u = q.front(); q.pop();
            std::list<int> children = g.getChildren(u);
            for(auto it = children.begin(); it != children.end(); it++) {
                int v = *it;
                if (!visited[v]) {
                    visited[v] = true;
                    q.push(v);
                    parents[v] = u;
                } else if (parents[u] != v) {
                    delete[] parents;
                    return true;
                }
            }
        }
    } else {
        while (!q.empty()) {
            u = q.front(); q.pop();
            std::list<int> neighbours = g.getNeighbours(u);
            for(auto it = neighbours.begin(); it != neighbours.end(); it++) {
                int v = *it;
                if (!visited[v]) {
                    visited[v] = true;
                    q.push(v);
                    parents[v] = u;
                } else if (parents[u] != v) {
                    delete[] parents;
                    return true;
                }
            }
        }
    }
    delete[] parents;
    return false;
}

bool hasCycle(graph& g)
{
    int numVertices = g.getNumberVertices();
    bool* visited = new bool[numVertices];
    std::fill(visited, visited + numVertices, false);
    bool cycle = isCyclic(g, visited);
    delete[] visited;
    return cycle;
}

bool isTree(graph& g)
{
    int numVertices = g.getNumberVertices();
    bool* visited = new bool[numVertices];
    std::fill(visited, visited + numVertices, false);
    bool gHasCycle = isCyclic(g, visited);
    if (gHasCycle) {
        delete[] visited;
        return false;
    }
    for (int u = 0; u < numVertices; u++) {
       if (u != g.getRoot() && !visited[u]) {
           delete[] visited;
           return false;
       }
    }
    delete[] visited;
    return true;
}

GraphCycleType detectCycleType(graph& g)
{
    int numVertices = g.getNumberVertices();
    bool* visited = new bool[numVertices];
    std::fill(visited, visited + numVertices, false);
    bool gHasCycle = isCyclic(g, visited);
    if (gHasCycle) {
        delete[] visited;
        return GraphCycleType::CYCLE;
    }
    auto const check = [](bool x) { return x == true; };
    auto all = std::all_of(visited, visited + numVertices, check);
    if (!all) {
        delete[] visited;
        return GraphCycleType::DISCONNECTED;
    }
    delete[] visited;
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
                std::list<int> neighbors = g_.getNeighbours(v);
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
                std::list<int> neighbors = g_.getNeighbours(v);
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
