#include "ws/lib.hpp"
// #include "ws/hazard_pointers.hpp"


// things to implement

// - Support for memory recycle
// - Check how to manage references and tags to avoid the ABA problem
// - Memory management for complex objects

// For memory management, we should add the support for hazard pointers



//////////////////////////
// Task array with size //
//////////////////////////

taskArrayWithSize::taskArrayWithSize() : size(1), array(new std::atomic<int>[1]) {}

taskArrayWithSize::taskArrayWithSize(int size) : size(size) {
    array = new std::atomic<int>[size];
    std::fill(array, array + size, BOTTOM);
}

taskArrayWithSize::taskArrayWithSize(int size, int defaultValue) : size(size) {
    array = new std::atomic<int>[size];
    std::fill(array, array + size, defaultValue);
}

taskArrayWithSize::taskArrayWithSize(const taskArrayWithSize& other) : size(other.size) {
    array = new std::atomic<int>[other.size];
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

idempotentDeque::idempotentDeque(int size) : capacity(size),
                                             tasks(taskArrayWithSize(size)){}

bool idempotentDeque::isEmpty() {
    return anchor.load()->size == 0;
}

bool idempotentDeque::put(int task) {
    // std::atomic<void*>& hp
    // auto old_anchor = anchor.load();
    auto old_anchor = anchor.load();
    auto[head, size, tag] = *old_anchor;
    if (size == tasks.getSize()) {
        expand();
        return put(task);
    }
    tasks.set((head + size) % tasks.getSize(), task);
    std::atomic_thread_fence(std::memory_order_release);
    anchor.store(new triplet{head, size + 1, tag + 1});
    // delete old_anchor;
    // std::atomic<void*>& hp = get_hazard_pointer_for_current_thread();
    // triplet* old_anchor = anchor.load();
    // do {
    //     triplet* temp;
    //     do {
    //         temp = old_anchor;
    //         hp.store(old_anchor);
    //         old_anchor = anchor.load();
    //     } while (old_anchor != temp);
    // } while(old_anchor &&
    //         anchor.compare_exchange_strong(old_anchor,
    //                                        new triplet(head, size + 1, tag + 1)));
    // hp.store(nullptr);
    // if (old_anchor) {

    //     if (outstanding_hazard_pointers_for(old_anchor)) {
    //         reclaim_later(old_anchor);
    //     } else {
    //         delete old_anchor;
    //     }
    //     delete_nodes_with_no_hazards();
    // }
    return true;
}

int idempotentDeque::take() {
    // std::atomic<void*>& hp = get_hazard_pointer_for_current_thread();
    // triplet* old_anchor = anchor.load();
    // do {
    //     triplet* temp;
    //     do {
    //         temp = old_anchor;
    //         hp.store(old_anchor);
    //         old_anchor = anchor.load();
    //     } while(old_anchor != temp);
    // } while (old_anchor && anchor.compare_exchange_strong(old_anchor, new triplet(head, size + 1, )))
    // triplet* temp;
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
            // delete oldReference;
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


//////////////////////////////////////////////
// Idempotent DEQUE work-stealing algorithm //
//////////////////////////////////////////////

idempotentDeque2::idempotentDeque2(int size) : capacity(size),
                                               tasks(taskArrayWithSize(size)){}

bool idempotentDeque2::isEmpty() {
    unsigned long long value = anchor.load();
    return ((value >> 16) & 0xFFFFFF) == 0;
}

bool idempotentDeque2::put(int task) {
    unsigned long long value = anchor.load();
    auto head = (value >> 40) & 0xFFFFFF;
    auto size = (value >> 16) & 0xFFFFFF;
    auto tag = value & 0xFFFF;
    // auto[head, size, tag] = anchor.load();
    if ((int)size == tasks.getSize()) {
        expand();
        return put(task);
    }
    tasks.set((head + size) % tasks.getSize(), task);
    std::atomic_thread_fence(std::memory_order_release);
    unsigned long long newValue = (tag + 1) | (size + 1) << 16 | head << 40;
    anchor.store(newValue);
    return true;
}

int idempotentDeque2::take() {
    unsigned long long value = anchor.load();
    auto head = (value >> 40) & 0xFFFFFF;
    auto size = (value >> 16) & 0xFFFFFF;
    auto tag = value & 0xFFFF;
    if (size == 0) return EMPTY;
    int task = tasks.get((head + size - 1) % tasks.getSize());
    unsigned long long newValue = tag | (size - 1) << 16 | head << 40;
    anchor.store(newValue);
    return task;
}

int idempotentDeque2::steal() {
    while (true) {
        // auto oldReference = anchor.load();
        unsigned long long value = anchor.load();
        auto head = (value >> 40) & 0xFFFFFF;
        auto size = (value >> 16) & 0xFFFFFF;
        auto tag = value & 0xFFFF;
        // auto[head, size, tag] = oldReference;
        if (size == 0) return EMPTY;
        std::atomic_thread_fence(std::memory_order_acquire);
        taskArrayWithSize* a = &tasks;
        auto task = a->get(head % a->getSize());
        unsigned long long h2 = (head + 1) % a->getSize();
        unsigned long long newValue = tag | (size - 1) << 16 | h2 << 40;
        if (anchor.compare_exchange_strong(value, newValue)) {
            return task;
        }
    }
}

void idempotentDeque2::expand() {
    unsigned long long value = anchor.load();
    auto head = (value >> 40) & 0xFFFFFF;
    auto size = (value >> 16) & 0xFFFFFF;
    taskArrayWithSize a(2 * tasks.getSize());
    std::atomic_thread_fence(std::memory_order_release);
    for (int i = 0; i < (int)size; i++) {
        a.set((head + i) % a.getSize(), tasks.get((head + i) % tasks.getSize()));
        std::atomic_thread_fence(std::memory_order_release);
    }
    tasks = std::move(a);
    std::atomic_thread_fence(std::memory_order_seq_cst);
}

int idempotentDeque2::getSize() {
    return tasks.getSize();
}
