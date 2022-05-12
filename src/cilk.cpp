#include "ws/lib.hpp"

//////////////////////////////////
// Cilk work-stealing algorithm //
//////////////////////////////////

cilk::cilk(int initialSize) : tasks (new std::atomic<int>[initialSize]) {
    H = 0;
    T = 0;
    tasksSize = initialSize;
    std::fill(tasks.get(), tasks.get() + initialSize, BOTTOM);
}

bool cilk::isEmpty() {
    int tail = T.load();
    int head = H.load();
    return head >= tail;
}

void cilk::expand() {
    int newSize = 2 * tasksSize;
    auto *newData = new std::atomic<int>[newSize];
    for (int i = 0; i < tasksSize; i++) newData[i] = tasks[i].load();
    tasks.reset(newData);
    tasksSize = newSize;
}

int cilk::getSize() {
    return tasksSize;
}

bool cilk::put(int task) {
    int tail = T.load(relaxed);
    if (tail == tasksSize) {
        expand();
        return put(task);
    }
    tasks[mod(tail, tasksSize)] = task;
    std::atomic_thread_fence(std::memory_order_release);
    T.store(tail + 1, relaxed);
    return true;
}

int cilk::take() {
    int tail = T.load(relaxed) - 1;
    T.store(tail, relaxed);
    std::atomic_thread_fence(std::memory_order_seq_cst);
    int head = H.load(relaxed);

    if (tail > head) return tasks[mod(tail, tasksSize)];
    if (tail < head) {
        const std::lock_guard<std::mutex> lock(mtx);
        if (H.load() >= (tail + 1)) {
            T.store(tail + 1, relaxed);
            return EMPTY;
        }
    }
    return tasks[mod(tail, tasksSize)];
}

int cilk::steal() {
    int ret;
    const std::lock_guard<std::mutex> lock(mtx);
    int h = H.load(relaxed);
    H.store(h + 1, relaxed);
    std::atomic_thread_fence(seq_cst);
    if ((h + 1) <= T.load(acquire)) {
        ret = tasks[mod(h, tasksSize)];
    } else {
        H.store(h, relaxed);
        ret = EMPTY;
    }
    return ret;
}
