#include "ws/lib.hpp"

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
        if (anchor.compare_exchange_strong(oldReference, new triplet(h2, size - 1, tag), std::memory_order_acq_rel)) {
            delete oldReference;
            return task;
        }
    }
}

void idempotentDeque::expand() {
    triplet *old = anchor.load();
    auto[head, size, tag] = *old;
    taskArrayWithSize a(2 * tasks.getSize());
    std::atomic_thread_fence(std::memory_order_release);
    for (int i = 0; i < size; i++) {
        a.set((head + i) % a.getSize(), tasks.get((head + i) % tasks.getSize()));
        std::atomic_thread_fence(std::memory_order_release);
    }
    tasks = a;
    std::atomic_thread_fence(std::memory_order_seq_cst);
}

int idempotentDeque::getSize() {
    return tasks.getSize();
}
