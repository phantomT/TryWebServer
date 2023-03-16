#ifndef BLOCK_QUEUE_H
#define BLOCK_QUEUE_H

#include <mutex>
#include <deque>
#include <condition_variable>
#include <sys/time.h>

template<class T>
class BlockDeque {
public:
    explicit BlockDeque(size_t MaxCapacity = 1000);

    ~BlockDeque();

    void Clear();

    bool Empty();

    bool Full();

    void Close();

    size_t Size();

    size_t Capacity();

    T &Front();

    T &Back();

    void Push_back(const T &item);

    void Push_front(const T &item);

    bool Pop(T &item);

    bool Pop(T &item, int timeout);

    void Flush();

private:
    bool isClose;
    size_t b_capacity;
    std::deque<T> b_deq;
    std::mutex b_mutex;
    std::condition_variable condCons;
    std::condition_variable condProd;
};


template<class T>
BlockDeque<T>::BlockDeque(size_t MaxCapacity) :b_capacity(MaxCapacity) {
    assert(MaxCapacity > 0);
    isClose = false;
}

template<class T>
BlockDeque<T>::~BlockDeque() {
    Close();
}

template<class T>
void BlockDeque<T>::Close() {
    {
        std::lock_guard<std::mutex> locker(b_mutex);
        b_deq.clear();
        isClose = true;
    }
    condProd.notify_all();
    condCons.notify_all();
}

template<class T>
void BlockDeque<T>::Flush() {
    condCons.notify_one();
}

template<class T>
void BlockDeque<T>::Clear() {
    std::lock_guard<std::mutex> locker(b_mutex);
    b_deq.clear();
}

template<class T>
T &BlockDeque<T>::Front() {
    std::lock_guard<std::mutex> locker(b_mutex);
    return b_deq.front();
}

template<class T>
T &BlockDeque<T>::Back() {
    std::lock_guard<std::mutex> locker(b_mutex);
    return b_deq.back();
}

template<class T>
size_t BlockDeque<T>::Size() {
    std::lock_guard<std::mutex> locker(b_mutex);
    return b_deq.size();
}

template<class T>
size_t BlockDeque<T>::Capacity() {
    std::lock_guard<std::mutex> locker(b_mutex);
    return b_capacity;
}

template<class T>
void BlockDeque<T>::Push_back(const T &item) {
    std::unique_lock<std::mutex> locker(b_mutex);
    while (b_deq.size() >= b_capacity) {
        condProd.wait(locker);
    }
    b_deq.emplace_back(item);
    condCons.notify_one();
}

template<class T>
void BlockDeque<T>::Push_front(const T &item) {
    std::unique_lock<std::mutex> locker(b_mutex);
    while (b_deq.size() >= b_capacity) {
        condProd.wait(locker);
    }
    b_deq.emplace_front(item);
    condCons.notify_one();
}

template<class T>
bool BlockDeque<T>::Empty() {
    std::lock_guard<std::mutex> locker(b_mutex);
    return b_deq.empty();
}

template<class T>
bool BlockDeque<T>::Full() {
    std::lock_guard<std::mutex> locker(b_mutex);
    return b_deq.size() >= b_capacity;
}

template<class T>
bool BlockDeque<T>::Pop(T &item) {
    std::unique_lock<std::mutex> locker(b_mutex);
    while (b_deq.empty()) {
        condCons.wait(locker);
        if (isClose) {
            return false;
        }
    }
    item = b_deq.front();
    b_deq.pop_front();
    condProd.notify_one();
    return true;
}

template<class T>
bool BlockDeque<T>::Pop(T &item, int timeout) {
    std::unique_lock<std::mutex> locker(b_mutex);
    while (b_deq.empty()) {
        if (condCons.wait_for(locker, std::chrono::seconds(timeout))
            == std::cv_status::timeout) {
            return false;
        }
        if (isClose) {
            return false;
        }
    }
    item = b_deq.front();
    b_deq.pop_front();
    condProd.notify_one();
    return true;
}

#endif // BLOCK_QUEUE_H