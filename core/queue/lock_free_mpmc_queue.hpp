#pragma once

#include <atomic>
#include <cstddef>
#include <memory>
#include <type_traits>
#include <utility>

namespace nexusflow {

template <typename T>
class LockFreeMPMCQueue {
public:
    explicit LockFreeMPMCQueue(size_t capacity)
        : capacity_(capacity),
          buffer_(std::make_unique<Cell[]>(capacity)),
          enqueue_pos_(0),
          dequeue_pos_(0)
    {
        for (size_t i = 0; i < capacity_; ++i) {
            buffer_[i].sequence.store(
                i,
                std::memory_order_relaxed
            );
        }
    }

    LockFreeMPMCQueue(const LockFreeMPMCQueue&) = delete;
    LockFreeMPMCQueue& operator=(const LockFreeMPMCQueue&) = delete;

    bool try_push(const T& value)
    {
        return enqueue(value);
    }

    bool try_push(T&& value)
    {
        return enqueue(std::move(value));
    }

    bool try_pop(T& value)
    {
        size_t position =
            dequeue_pos_.load(std::memory_order_relaxed);

        while (true) {
            Cell& cell = buffer_[position % capacity_];

            const size_t sequence =
                cell.sequence.load(std::memory_order_acquire);

            const intptr_t difference =
                static_cast<intptr_t>(sequence) -
                static_cast<intptr_t>(position + 1);

            if (difference == 0) {
                if (dequeue_pos_.compare_exchange_weak(
                        position,
                        position + 1,
                        std::memory_order_relaxed)) {

                    value = std::move(cell.storage);
                    cell.sequence.store(
                        position + capacity_,
                        std::memory_order_release
                    );

                    return true;
                }
            }
            else if (difference < 0) {
                return false;
            }
            else {
                position =
                    dequeue_pos_.load(
                        std::memory_order_relaxed
                    );
            }
        }
    }

    size_t capacity() const
    {
        return capacity_;
    }

private:
    struct Cell {
        std::atomic<size_t> sequence{0};
        T storage{};
    };

    template <typename U>
    bool enqueue(U&& value)
    {
        size_t position =
            enqueue_pos_.load(std::memory_order_relaxed);

        while (true) {
            Cell& cell = buffer_[position % capacity_];

            const size_t sequence =
                cell.sequence.load(std::memory_order_acquire);

            const intptr_t difference =
                static_cast<intptr_t>(sequence) -
                static_cast<intptr_t>(position);

            if (difference == 0) {
                if (enqueue_pos_.compare_exchange_weak(
                        position,
                        position + 1,
                        std::memory_order_relaxed)) {

                    cell.storage = std::forward<U>(value);

                    cell.sequence.store(
                        position + 1,
                        std::memory_order_release
                    );

                    return true;
                }
            }
            else if (difference < 0) {
                return false;
            }
            else {
                position =
                    enqueue_pos_.load(
                        std::memory_order_relaxed
                    );
            }
        }
    }

    const size_t capacity_;
    std::unique_ptr<Cell[]> buffer_;

    alignas(64)
    std::atomic<size_t> enqueue_pos_;

    alignas(64)
    std::atomic<size_t> dequeue_pos_;
};

}
