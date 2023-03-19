#include "timer.h"

void Timer::ShiftUp(size_t i) {
    assert(i >= 0 && i < t_heap.size());
    size_t j = (i - 1) / 2;
    while (!(t_heap[j] < t_heap[i])) {
        SwapNode(i, j);
        i = j;
        j = (i - 1) / 2;
    }
}

void Timer::SwapNode(size_t i, size_t j) {
    assert(i >= 0 && i < t_heap.size());
    assert(j >= 0 && j < t_heap.size());
    std::swap(t_heap[i], t_heap[j]);
    t_ref[t_heap[i].id] = i;
    t_ref[t_heap[j].id] = j;
}

bool Timer::ShiftDown(size_t index, size_t n) {
    assert(index >= 0 && index < t_heap.size());
    assert(n >= 0 && n <= t_heap.size());
    size_t i = index;
    size_t j = i * 2 + 1;
    while (j < n) {
        if (j + 1 < n && t_heap[j + 1] < t_heap[j]) j++;
        if (t_heap[i] < t_heap[j]) break;
        SwapNode(i, j);
        i = j;
        j = i * 2 + 1;
    }
    return i > index;
}

void Timer::Add(int id, int timeOut, const TimeoutCallBack &cb) {
    assert(id >= 0);
    size_t i;
    if (t_ref.count(id) == 0) {
        // 新节点：堆尾插入，调整堆
        i = t_heap.size();
        t_ref[id] = i;
        t_heap.push_back({id, Clock::now() + MS(timeOut), cb});
        if(i) ShiftUp(i);
    } else {
        // 已有结点：调整堆
        i = t_ref[id];
        t_heap[i].expires = Clock::now() + MS(timeOut);
        t_heap[i].cb = cb;
        if (!ShiftDown(i, t_heap.size())) {
            ShiftUp(i);
        }
    }
}

void Timer::DoWork(int id) {
    // 删除指定id结点，并触发回调函数
    if (t_heap.empty() || t_ref.count(id) == 0) {
        return;
    }
    size_t i = t_ref[id];
    TimerNode node = t_heap[i];
    node.cb();
    Del(i);
}

void Timer::Del(size_t index) {
    // 删除指定位置的结点
    assert(!t_heap.empty() && index >= 0 && index < t_heap.size());
    // 将要删除的结点换到队尾，然后调整堆
    size_t i = index;
    size_t n = t_heap.size() - 1;
    assert(i <= n);
    if (i < n) {
        SwapNode(i, n);
        if (!ShiftDown(i, n)) {
            ShiftUp(i);
        }
    }
    // 队尾元素删除
    t_ref.erase(t_heap.back().id);
    t_heap.pop_back();
}

void Timer::Adjust(int id, int timeout) {
    // 调整指定id的结点
    assert(!t_heap.empty() && t_ref.count(id) > 0);
    t_heap[t_ref[id]].expires = Clock::now() + MS(timeout);
    ShiftDown(t_ref[id], t_heap.size());
}

void Timer::Tick() {
    // 清除超时结点
    if (t_heap.empty()) {
        return;
    }
    while (!t_heap.empty()) {
        TimerNode node = t_heap.front();
        if (std::chrono::duration_cast<MS>(node.expires - Clock::now()).count() > 0) {
            break;
        }
        node.cb();
        Pop();
    }
}

void Timer::Pop() {
    assert(!t_heap.empty());
    Del(0);
}

void Timer::Clear() {
    t_ref.clear();
    t_heap.clear();
}

int Timer::GetNextTick() {
    Tick();
    size_t res = -1;
    if (!t_heap.empty()) {
        res = std::chrono::duration_cast<MS>(t_heap.front().expires - Clock::now()).count();
        if (res < 0) { res = 0; }
    }
    return res;
}