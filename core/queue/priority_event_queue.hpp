#pragma once

#include <array>
#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <queue>
#include <utility>

#include "core/event/event.hpp"

namespace nexusflow {

class PriorityEventQueue {
public:
    explicit PriorityEventQueue(
        std::size_t capacity
    )
        : capacity_(capacity) {
    }

    PriorityEventQueue(
        const PriorityEventQueue&
    ) = delete;

    PriorityEventQueue& operator=(
        const PriorityEventQueue&
    ) = delete;

    bool try_push(const Event& event) {
        std::lock_guard<std::mutex> lock(mutex_);

        if (closed_ || size_ >= capacity_) {
            return false;
        }

        queues_[priority_index(event.priority)].push(event);

        ++size_;

        not_empty_.notify_one();

        return true;
    }

    bool try_push(Event&& event) {
        std::lock_guard<std::mutex> lock(mutex_);

        if (closed_ || size_ >= capacity_) {
            return false;
        }

        const std::size_t index =
            priority_index(event.priority);

        queues_[index].push(
            std::move(event)
        );

        ++size_;

        not_empty_.notify_one();

        return true;
    }

    bool try_pop(Event& event) {
        std::lock_guard<std::mutex> lock(mutex_);

        if (size_ == 0) {
            return false;
        }

        for (std::size_t index = 0;
             index < queues_.size();
             ++index) {

            if (!queues_[index].empty()) {
                event =
                    std::move(
                        queues_[index].front()
                    );

                queues_[index].pop();

                --size_;

                return true;
            }
        }

        return false;
    }

    bool pop(Event& event) {
        std::unique_lock<std::mutex> lock(mutex_);

        not_empty_.wait(
            lock,
            [this] {
                return size_ > 0 || closed_;
            }
        );

        if (size_ == 0) {
            return false;
        }

        for (std::size_t index = 0;
             index < queues_.size();
             ++index) {

            if (!queues_[index].empty()) {
                event =
                    std::move(
                        queues_[index].front()
                    );

                queues_[index].pop();

                --size_;

                return true;
            }
        }

        return false;
    }

    void close() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            closed_ = true;
        }

        not_empty_.notify_all();
    }

    bool closed() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return closed_;
    }

    bool empty() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return size_ == 0;
    }

    std::size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return size_;
    }

    std::size_t capacity() const {
        return capacity_;
    }

private:
    static std::size_t priority_index(
        EventPriority priority
    ) {
        switch (priority) {
            case EventPriority::CRITICAL:
                return 0;

            case EventPriority::HIGH:
                return 1;

            case EventPriority::NORMAL:
                return 2;

            case EventPriority::LOW:
                return 3;
        }

        return 2;
    }

    std::array<
        std::queue<Event>,
        4
    > queues_;

    std::size_t capacity_;

    mutable std::mutex mutex_;

    std::condition_variable not_empty_;

    std::size_t size_{0};

    bool closed_{false};
};

} // namespace nexusflow
