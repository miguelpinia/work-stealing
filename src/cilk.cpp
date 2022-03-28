#include "ws/lib.hpp"

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
