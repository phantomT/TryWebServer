#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <mutex>
#include <condition_variable>
#include <queue>
#include <thread>
#include <functional>

class ThreadPool {
public:
    explicit ThreadPool(size_t threadCount = 8) : t_pool(std::make_shared<Pool>()) {
        assert(threadCount > 0);
        for (size_t i = 0; i < threadCount; ++i) {
            std::thread([pool = t_pool] {
                std::unique_lock<std::mutex> locker(pool->p_mutex);
                while (true) {
                    if (!pool->tasks.empty()) {
                        auto task = std::move(pool->tasks.front());
                        pool->tasks.pop();
                        locker.unlock();
                        task();
                        locker.lock();
                    } else if (pool->isClosed) break;
                    else pool->cond.wait(locker);
                }
            }).detach();
        }
    }

    ThreadPool() = default;

    ThreadPool(ThreadPool &&) = default;

    ~ThreadPool() {
        if (static_cast<bool>(t_pool)) {
            {
                std::lock_guard<std::mutex> locker(t_pool->p_mutex);
                t_pool->isClosed = true;
            }
            t_pool->cond.notify_all();
        }
    }

    template<class T>
    void AddTask(T &&task) {
        {
            std::lock_guard<std::mutex> locker(t_pool->p_mutex);
            t_pool->tasks.emplace(std::forward<T>(task));
        }
        t_pool->cond.notify_one();
    }

private:
    struct Pool {
        bool isClosed{};
        std::mutex p_mutex;
        std::condition_variable cond;
        std::queue<std::function<void()>> tasks;
    };
    std::shared_ptr<Pool> t_pool;
};


#endif //THREAD_POOL_H