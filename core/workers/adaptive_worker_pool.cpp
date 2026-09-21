#include "core/workers/adaptive_worker_pool.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace nexusflow {

AdaptiveWorkerPool::AdaptiveWorkerPool(
    BoundedMPMCQueue<Event>& queue,
    std::size_t max_workers,
    Processor processor
)
    : queue_(queue),
      max_workers_(
          std::max<std::size_t>(1, max_workers)
      ),
      processor_(std::move(processor)) {

    if (!processor_) {
        throw std::invalid_argument(
            "AdaptiveWorkerPool requires a valid processor."
        );
    }
}

AdaptiveWorkerPool::~AdaptiveWorkerPool() {
    stop();
}

void AdaptiveWorkerPool::start() {
    bool expected = false;

    if (!started_.compare_exchange_strong(
            expected,
            true
        )) {
        return;
    }

    running_.store(
        true,
        std::memory_order_release
    );

    workers_.reserve(max_workers_);

    for (
        std::size_t worker_id = 0;
        worker_id < max_workers_;
        ++worker_id
    ) {
        workers_.emplace_back(
            &AdaptiveWorkerPool::worker_loop,
            this,
            worker_id
        );
    }
}

void AdaptiveWorkerPool::set_active_workers(
    std::size_t worker_count
) {
    if (worker_count < 1) {
        worker_count = 1;
    }

    if (worker_count > max_workers_) {
        worker_count = max_workers_;
    }

    active_workers_.store(
        worker_count,
        std::memory_order_release
    );
}

std::size_t AdaptiveWorkerPool::active_workers() const {
    return active_workers_.load(
        std::memory_order_acquire
    );
}

std::size_t AdaptiveWorkerPool::max_workers() const {
    return max_workers_;
}

std::uint64_t AdaptiveWorkerPool::processed_events() const {
    return processed_events_.load(
        std::memory_order_relaxed
    );
}

std::uint64_t AdaptiveWorkerPool::idle_cycles() const {
    return idle_cycles_.load(
        std::memory_order_relaxed
    );
}

bool AdaptiveWorkerPool::running() const {
    return running_.load(
        std::memory_order_acquire
    );
}

void AdaptiveWorkerPool::wait_until_processed(
    std::uint64_t expected_count
) {
    while (
        processed_events_.load(
            std::memory_order_acquire
        ) < expected_count
    ) {
        std::this_thread::yield();
    }
}

void AdaptiveWorkerPool::stop() {
    bool expected = true;

    if (!started_.compare_exchange_strong(
            expected,
            false
        )) {
        return;
    }

    running_.store(
        false,
        std::memory_order_release
    );

    for (auto& worker : workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }

    workers_.clear();
}

void AdaptiveWorkerPool::worker_loop(
    std::size_t worker_id
) {

    while (
        running_.load(
            std::memory_order_acquire
        )
    ) {

        const std::size_t active =
            active_workers_.load(
                std::memory_order_acquire
            );

        if (worker_id >= active) {

            idle_cycles_.fetch_add(
                1,
                std::memory_order_relaxed
            );

            std::this_thread::yield();

            continue;
        }

        const std::size_t configured_batch =
            batch_size_.load(
                std::memory_order_acquire
            );

        const std::size_t batch_limit =
            std::max<std::size_t>(
                1,
                configured_batch
            );

        std::vector<Event> batch;
        batch.reserve(batch_limit);

        Event event;

        if (!queue_.try_pop(event)) {

            idle_cycles_.fetch_add(
                1,
                std::memory_order_relaxed
            );

            std::this_thread::yield();

            continue;
        }

        batch.push_back(
            std::move(event)
        );

        while (
            batch.size() < batch_limit &&
            queue_.try_pop(event)
        ) {

            batch.push_back(
                std::move(event)
            );
        }

        for (
            const auto& item : batch
        ) {

            processor_(item);

            processed_events_.fetch_add(
                1,
                std::memory_order_relaxed
            );
        }
    }
}


void AdaptiveWorkerPool::set_batch_size(
    std::size_t batch_size
) {

    if (batch_size == 0) {
        batch_size = 1;
    }

    batch_size_.store(
        batch_size,
        std::memory_order_release
    );
}

std::size_t
AdaptiveWorkerPool::batch_size() const {

    return batch_size_.load(
        std::memory_order_acquire
    );
}
} // namespace nexusflow


