#include "ws/lib.hpp"
#include <algorithm>
#include <iostream>

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
                                                                                                  numVertices(
                                                                                                          numVertices),
                                                                                                  type(type) {
    for (auto i = 0; i < numVertices; i++) vertices.emplace_back(vertex(directed, i));
    for (auto edge: edges) addEdge(edge);
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
    array = std::vector<int>(size, -1);
}

taskArrayWithSize::taskArrayWithSize(int size, int defaultValue) : size(size) {
    array = std::vector<int>(size, defaultValue);
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
    tasks = a;
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
    tasks = new int[size];
    capacity = size;
    pair *p = new pair(0, 0);
    anchor.store(p);
    for (int i = 0; i < size; i++) {
        tasks[i] = BOTTOM;
    }

}

bool idempotentLIFO::isEmpty() {
    return anchor.load()->g == 0;
}

bool idempotentLIFO::put(int task) {
    auto oldReference = anchor.load();
    auto[t, g] = *oldReference;
    if (t == capacity) expand();
    tasks[t] = task;
    std::atomic_thread_fence(std::memory_order_release);
    pair *newPair = new pair(t + 1, g + 1);
    anchor.store(newPair);
    delete oldReference;
    return true;
}

int idempotentLIFO::take() {
    auto oldReference = anchor.load();
    auto[t, g] = *oldReference;
    if (t == 0) return EMPTY;
    int task = tasks[t - 1];
    pair *newPair = new pair(t - 1, g);
    anchor.store(newPair);
    delete oldReference;
    return task;
}

int idempotentLIFO::steal() {
    while (true) {
        pair *oldReference = anchor.load();
        auto[t, g] = *oldReference;
        if (t == 0) return EMPTY;
        int *tmp = tasks;
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
    delete[] tasks;
    this->tasks = newTasks;
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
    triplet *old = anchor.load();
    auto[head, size, tag] = *old;
    if (size == tasks.getSize()) expand();
    tasks.set((head + size) % tasks.getSize(), task);
    std::atomic_thread_fence(std::memory_order_release);
    auto *t = new triplet(head, size + 1, tag + 1);
    anchor.store(t);
    delete old;
    return true;
}

int idempotentDeque::take() {
    triplet *old = anchor.load();
    auto[head, size, tag] = *old;
    if (size == 0) return EMPTY;
    int task = tasks.get((head + size - 1) % tasks.getSize());
    auto *t = new triplet(head, size - 1, tag);
    anchor.store(t);
    delete old;
    return task;
}

int idempotentDeque::steal() {
    while (true) {
        auto oldReference = anchor.load();
        auto[head, size, tag] = *oldReference;
        if (size == 0) return EMPTY;
        std::atomic_thread_fence(std::memory_order_acquire);
        taskArrayWithSize a = tasks;
        auto task = a.get(head % a.getSize());
        auto h2 = (head + 1) % a.getSize();
        std::atomic_thread_fence(std::memory_order_acquire);
        auto *newRef = new triplet(h2, size - 1, tag);
        if (anchor.compare_exchange_strong(oldReference, newRef)) {
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
        a.set((head + 1) % a.getSize(), tasks.get((head + 1) % tasks.getSize()));
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

wsncmult::wsncmult(int size, int numThreads) : tail(-1), size(size) {
    Head = 0;
    head = std::vector<int>(numThreads, 0);
    tasks = std::vector<int>(size, BOTTOM);
}

bool wsncmult::isEmpty(int label) {
    (void) label;
    return Head.load() > tail;
}

bool wsncmult::put(int task, int label) {
    (void) label;
    tail++;
    if (tail == (int) tasks.size() - 1) expand();
    if (tail <= (int) tasks.size() - 3) {
        tasks[tail + 1] = BOTTOM;
        tasks[tail + 2] = BOTTOM;
    }
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

void wsncmult::expand() {}


////////////////////////////////////////////////////
// Bounded work-stealing algorithm implementation //
////////////////////////////////////////////////////

// bwsncmult::bwsncmult(int capacity, int numThreads) : tail(-1), capacity(capacity)
// {
//     Head = 0;
//     head = new int[numThreads];
//     tasks = new int[capacity];
//     B = new bool[capacity];
//     for (int i = 0; i < numThreads; i++) {
//         head[i] = 0;
//     }
//     for (int i = 0; i < capacity; i++) {
//         tasks[i] = 0;
//         B[i] = false;
//     }
// }

// bool bwsncmult::isEmpty(int label)
// {
//     (void) label;
//     return Head.load() > tail;
// }

// bool bwsncmult::put(int task, int label)
// {
//     if (tail == capacity) {
//         expand();
//     }
//     if (tail <= capacity - 3) {
//         tasks[tail + 1] = BOTTOM;
//         tasks[tail + 1] = BOTTOM;

//     }
// }
