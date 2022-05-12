#include "ws/lib.hpp"

///////////////////////////////////////
// Chase-Lev work-stealing algorithm //
///////////////////////////////////////

// We're following the description provided by Morrison and Afek from
// the article "Fence-Free Work Stealing on Bounded TSO Processors" to
// implement Chase-Lev work-stealing algorithm
chaselev::chaselev(int initialSize) : tasks(new std::atomic<int>[initialSize]){
    H = 0;
    T = 0;
    tasksSize = initialSize;
    std::fill(tasks.get(), tasks.get() + initialSize, BOTTOM);
}

bool chaselev::isEmpty() {
    int tail = T.load();
    int head = H.load();
    return head >= tail;
}

void chaselev::expand() {
    int newSize = 2 * tasksSize;
    auto *newData = new std::atomic<int>[newSize];
    for (int i = 0; i < tasksSize; i++) newData[i] = tasks[i].load();
    tasks.reset(newData);
    tasksSize = newSize;
}

bool chaselev::put(int task) {
    int tail = T.load();
    if (tail == tasksSize) {
        expand();
        return put(task);
    }
    tasks[mod(tail, tasksSize)] = task;
    std::atomic_thread_fence(seq_cst);
    T.store(tail + 1);
    return true;
}

int chaselev::take() {
    int tail = T.load() - 1;
    T.store(tail);
    // In C++, the language doesn't have support for StoreLoad
    // fence. But using atomic thread fence with memory_order_seq_cst,
    // it's possible that compiler would add MFENCE fence.
    std::atomic_thread_fence(seq_cst);
    int h = H.load();
    if (tail > h) return tasks[mod(tail, tasksSize)];
    if (tail < h) {
        T.store(h);
        return EMPTY;
    }
    T.store(h + 1);
    if (!H.compare_exchange_strong(h, h + 1, seq_cst, relaxed)) {
        return EMPTY;
    } else {
        return tasks[mod(tail, tasksSize)];
    }
}

int chaselev::steal() {
    while (true) {
        int h = H.load();
        std::atomic_thread_fence(seq_cst);
        int t = T.load();
        if (h >= t) return EMPTY;
        int task = tasks[mod(h, tasksSize)];
        if (!H.compare_exchange_strong(h, h + 1, seq_cst, relaxed)) {
            continue;
        }
        return task;
    }
}

int chaselev::getSize() {
    return tasksSize;
}
