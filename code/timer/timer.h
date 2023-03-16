#ifndef TIMER_H
#define TIMER_H

#include <queue>
#include <unordered_map>
#include <ctime>
#include <algorithm>
#include <arpa/inet.h>
#include <functional>
#include <cassert>
#include <chrono>

#include "../log/log.h"

typedef std::function<void()> TimeoutCallBack;
typedef std::chrono::high_resolution_clock Clock;
typedef std::chrono::milliseconds MS;
typedef Clock::time_point TimeStamp;

struct TimerNode {
    int id;
    TimeStamp expires;
    TimeoutCallBack cb;

    bool operator<(const TimerNode &t) const {
        return expires < t.expires;
    }
};

class Timer {
public:
    Timer() { t_heap.reserve(64); }

    ~Timer() { Clear(); }

    void Adjust(int id, int timeout);

    void Add(int id, int timeOut, const TimeoutCallBack &cb);

    void DoWork(int id);

    void Clear();

    void Tick();

    void Pop();

    int GetNextTick();

private:
    void Del(size_t index);

    void ShiftUp(size_t i);

    bool ShiftDown(size_t index, size_t n);

    void SwapNode(size_t i, size_t j);

    std::vector<TimerNode> t_heap;

    std::unordered_map<int, size_t> t_ref;
};

#endif //TIMER_H