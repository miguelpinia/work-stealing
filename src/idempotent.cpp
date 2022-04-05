#include "ws/lib.hpp"

//////////////////////////
// Task array with size //
//////////////////////////

taskArrayWithSize::taskArrayWithSize() : size(1), array(new std::atomic<int>[1]) {}

taskArrayWithSize::taskArrayWithSize(int size) : size(size), array(new std::atomic<int>[size]) {
    std::fill(array, array + size, BOTTOM);
}

taskArrayWithSize::taskArrayWithSize(int size, int defaultValue) : size(size), array(new std::atomic<int>[size]) {
    std::fill(array, array + size, defaultValue);
}

taskArrayWithSize::taskArrayWithSize(const taskArrayWithSize& other) : size(other.size), array(new std::atomic<int>[other.size]) {
    for (int i = 0; i < other.size; i++) array[i] = other.array[i].load();
}

taskArrayWithSize& taskArrayWithSize::operator=( const taskArrayWithSize& other )
{
    if (this == &other) {
        return *this;
    }
    if (size < other.size) {
        delete[] array;
        array = new std::atomic<int>[other.size];
    }
    size = other.size;
    for (int i = 0; i < size; i++) array[i] = other.array[i].load();
    return *this;
}

taskArrayWithSize::taskArrayWithSize(taskArrayWithSize &&other) : size(other.size), array(other.array) {
    other.size = 0;
    other.array = nullptr;
}

taskArrayWithSize& taskArrayWithSize::operator=(taskArrayWithSize &&other) {
    if (this == &other) {
        return *this;
    }
    size = other.size;
    delete[] array;
    array = other.array;
    other.size = 0;
    other.array = nullptr;
    return *this;
}

taskArrayWithSize::~taskArrayWithSize() {
    if (array != nullptr) delete[] array;
}

int &taskArrayWithSize::getSize() {
    return size;
}

int taskArrayWithSize::get(int position) {
    return array[position].load();
}

void taskArrayWithSize::set(int position, int value) {
    array[position].store(value);
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
    if (t == (h + tasks.getSize())) {
        expand();
        return put(task);
    }
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
        std::atomic_thread_fence(std::memory_order_release);
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
    tasks = taskArrayWithSize(size);
    capacity = size;
}

bool idempotentLIFO::isEmpty() {
    return anchor.load().t == 0;
}

bool idempotentLIFO::put(int task) {
    auto [t, g] = anchor.load();
    if (t == tasks.getSize()) {
        expand();
        return put(task);
    }
    tasks.set(t, task);
    std::atomic_thread_fence(release);
    anchor.store({t + 1, g + 1});
    return true;
}

int idempotentLIFO::take() {
    auto [t, g] = anchor.load();
    if (t == 0) return EMPTY;
    int task = tasks.get(t - 1);
    anchor.store({t - 1, g});
    return task;
}

int idempotentLIFO::steal() {
    while (true) {
        pair oldReference = anchor.load();
        auto[t, g] = oldReference;
        if (t == 0) {
            return EMPTY;
        }
        taskArrayWithSize *a = &tasks;
        int task = a->get(t - 1);
        std::atomic_thread_fence(acquire);
        pair newPair = {t - 1, g};
        if (anchor.compare_exchange_strong(oldReference, newPair)) {
            return task;
        }
    }
}

void idempotentLIFO::expand() {
    int size = tasks.getSize();
    taskArrayWithSize a(2 * size);
    std::atomic_thread_fence(std::memory_order_release);
    for (int i = 0; i < capacity; i++) {
        a.set(i, tasks.get(i));
        std::atomic_thread_fence(std::memory_order_release);
    }
    tasks = std::move(a);
    std::atomic_thread_fence(std::memory_order_seq_cst);
}

int idempotentLIFO::getSize() {
    return tasks.getSize();
}

//////////////////////////////////////////////
// Idempotent DEQUE work-stealing algorithm //
//////////////////////////////////////////////

idempotentDeque::idempotentDeque(int size) {
    capacity = size;
    tasks = taskArrayWithSize(size);
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
    anchor.store(new triplet{head, size + 1, tag + 1});
    return true;
}

int idempotentDeque::take() {
    auto [head, size, tag] = *anchor.load();
    if (size == 0) return EMPTY;
    int task = tasks.get((head + size - 1) % tasks.getSize());
    anchor.store(new triplet{head, size - 1, tag});
    return task;
}

int idempotentDeque::steal() {
    while (true) {
        auto oldReference = anchor.load();
        auto[head, size, tag] = *oldReference;
        if (size == 0) return EMPTY;
        std::atomic_thread_fence(std::memory_order_acquire);
        taskArrayWithSize* a = &tasks;
        auto task = a->get(head % a->getSize());
        auto h2 = (head + 1) % a->getSize();
        if (anchor.compare_exchange_strong(oldReference, new triplet{h2, size - 1, tag})) {
            return task;
        }
    }
}

void idempotentDeque::expand() {
    auto[head, size, tag] = *anchor.load();
    taskArrayWithSize a(2 * tasks.getSize());
    std::atomic_thread_fence(std::memory_order_release);
    for (int i = 0; i < size; i++) {
        a.set((head + i) % a.getSize(), tasks.get((head + i) % tasks.getSize()));
        std::atomic_thread_fence(std::memory_order_release);
    }
    tasks = std::move(a);
    std::atomic_thread_fence(std::memory_order_seq_cst);
}

int idempotentDeque::getSize() {
    return tasks.getSize();
}
