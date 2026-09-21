#pragma once

#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <queue>
#include <utility>

namespace nexusflow {

template <typename T>
class BoundedMPMCQueue {
public:
    explicit BoundedMPMCQueue(std::size_t capacity)
        : capacity_(capacity) {
    }

    bool push(const T& item) {
        std::unique_lock<std::mutex> lock(mutex_);

        not_full_.wait(
            lock,
            [this]() {
                return queue_.size() < capacity_ || closed_;
            }
        );

        if (closed_) {
            return false;
        }

        queue_.push(item);

        lock.unlock();
        not_empty_.notify_one();

        return true;
    }

    bool push(T&& item) {
        std::unique_lock<std::mutex> lock(mutex_);

        not_full_.wait(
            lock,
            [this]() {
                return queue_.size() < capacity_ || closed_;
            }
        );

        if (closed_) {
            return false;
        }

        queue_.push(std::move(item));

        lock.unlock();
        not_empty_.notify_one();

        return true;
    }

    bool try_push(const T& item) {
        std::lock_guard<std::mutex> lock(mutex_);

        if (closed_ || queue_.size() >= capacity_) {
            return false;
        }

        queue_.push(item);

        not_empty_.notify_one();

        return true;
    }

    bool try_push(T&& item) {
        std::lock_guard<std::mutex> lock(mutex_);

        if (closed_ || queue_.size() >= capacity_) {
            return false;
        }

        queue_.push(std::move(item));

        not_empty_.notify_one();

        return true;
    }

    bool pop(T& item) {
        std::unique_lock<std::mutex> lock(mutex_);

        not_empty_.wait(
            lock,
            [this]() {
                return !queue_.empty() || closed_;
            }
        );

        if (queue_.empty()) {
            return false;
        }

        item = std::move(queue_.front());
        queue_.pop();

        lock.unlock();
        not_full_.notify_one();

        return true;
    }

    bool try_pop(T& item) {
        std::lock_guard<std::mutex> lock(mutex_);

        if (queue_.empty()) {
            return false;
        }

        item = std::move(queue_.front());
        queue_.pop();

        not_full_.notify_one();

        return true;
    }

    void close() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            closed_ = true;
        }

        not_empty_.notify_all();
        not_full_.notify_all();
    }

    bool empty() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.empty();
    }

    std::size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size();
    }

    std::size_t capacity() const {
        return capacity_;
    }

    bool closed() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return closed_;
    }

private:
    mutable std::mutex mutex_;

    std::condition_variable not_empty_;
    std::condition_variable not_full_;

    std::queue<T> queue_;

    std::size_t capacity_;
    bool closed_{false};
};

} // namespace nexusflow
