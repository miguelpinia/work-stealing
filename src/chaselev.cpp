#include "ws/lib.hpp"

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
