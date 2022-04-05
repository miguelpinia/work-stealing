#include "ws/lib.hpp"

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
        tasks[i] = BOTTOM;
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
    if (tail == capacity - 1) expand();
    if (tail <= capacity - 3) {
        tasks[tail + 1] = BOTTOM;
        tasks[tail + 2] = BOTTOM;
        B[tail + 1] = true;
        B[tail + 2] = true;
    }
    tail++;
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
                    Head.store(h + 1);
                    return x;
                }
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
    std::fill(newData + capacity, newData + newCapacity, BOTTOM);
    auto newState = new std::atomic<bool>[newCapacity];
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
