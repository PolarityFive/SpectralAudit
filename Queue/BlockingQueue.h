#pragma once
#pragma once

#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <queue>

template <typename T>
class BlockingQueue {
public:
    explicit BlockingQueue(std::size_t capacity)
        : capacity(capacity) {
    }

    bool push(T item) {
        std::unique_lock<std::mutex> lock(mutex);

        notFullCv.wait(lock, [&] {
            return closed || queue.size() < capacity;
            });

        if (closed)
            return false;

        queue.push(std::move(item));
        notEmptyCv.notify_one();
        return true;
    }

    bool pop(T& out) {
        std::unique_lock<std::mutex> lock(mutex);

        notEmptyCv.wait(lock, [&] {
            return closed || !queue.empty();
            });

        if (queue.empty())
            return false;

        out = std::move(queue.front());
        queue.pop();
        notFullCv.notify_one();
        return true;
    }

    void close() {
        std::lock_guard<std::mutex> lock(mutex);
        closed = true;
        notEmptyCv.notify_all();
        notFullCv.notify_all();
    }

private:
    std::mutex mutex;
    std::condition_variable notEmptyCv;
    std::condition_variable notFullCv;
    std::queue<T> queue;
    std::size_t capacity;
    bool closed = false;
};
