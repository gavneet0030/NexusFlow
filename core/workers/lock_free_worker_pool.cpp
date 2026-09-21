#include "core/workers/lock_free_worker_pool.hpp"

#include <stdexcept>
#include <thread>
#include <utility>

namespace nexusflow {

LockFreeWorkerPool::LockFreeWorkerPool(
    std::size_t worker_count,
    std::size_t queue_capacity
)
    : queue_(queue_capacity),
      worker_count_(worker_count) {

    if (worker_count == 0) {
        throw std::invalid_argument(
            "Worker count must be greater than zero."
        );
    }

    if (queue_capacity == 0) {
        throw std::invalid_argument(
            "Queue capacity must be greater than zero."
        );
    }

    workers_.reserve(worker_count_);
    latency_samples_.resize(worker_count_);
}

LockFreeWorkerPool::~LockFreeWorkerPool() {
    shutdown();
}

void LockFreeWorkerPool::start() {
    bool expected = false;

    if (!started_.compare_exchange_strong(
            expected,
            true,
            std::memory_order_acq_rel
        )) {
        return;
    }

    for (std::size_t i = 0; i < worker_count_; ++i) {
        workers_.emplace_back(
            &LockFreeWorkerPool::worker_loop,
            this,
            i
        );
    }
}

bool LockFreeWorkerPool::submit(Event event) {
    if (!started_.load(std::memory_order_acquire)) {
        return false;
    }

    if (shutdown_.load(std::memory_order_acquire)) {
        return false;
    }

    return queue_.try_push(std::move(event));
}

void LockFreeWorkerPool::shutdown() {
    bool expected = false;

    if (!shutdown_.compare_exchange_strong(
            expected,
            true,
            std::memory_order_acq_rel
        )) {
        return;
    }

    for (auto& worker : workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }
}

void LockFreeWorkerPool::worker_loop(std::size_t worker_index) {
    EventProcessor processor;
    Event event;

    while (!shutdown_.load(std::memory_order_acquire)) {
        if (!queue_.try_pop(event)) {
            std::this_thread::yield();
            continue;
        }

        const ProcessingResult result =
            processor.process(event);

        latency_samples_[worker_index].push_back(result.latency_ns);

        processed_.fetch_add(
            1,
            std::memory_order_relaxed
        );

        total_latency_ns_.fetch_add(
            result.latency_ns,
            std::memory_order_relaxed
        );

        std::uint64_t current_max =
            max_latency_ns_.load(
                std::memory_order_relaxed
            );

        while (
            result.latency_ns > current_max &&
            !max_latency_ns_.compare_exchange_weak(
                current_max,
                result.latency_ns,
                std::memory_order_relaxed
            )
        ) {
        }

        double current_checksum =
            checksum_.load(
                std::memory_order_relaxed
            );

        while (
            !checksum_.compare_exchange_weak(
                current_checksum,
                current_checksum + result.result,
                std::memory_order_relaxed
            )
        ) {
        }
    }
}

std::uint64_t LockFreeWorkerPool::processed() const {
    return processed_.load(
        std::memory_order_relaxed
    );
}

std::uint64_t LockFreeWorkerPool::total_latency_ns() const {
    return total_latency_ns_.load(
        std::memory_order_relaxed
    );
}

std::uint64_t LockFreeWorkerPool::max_latency_ns() const {
    return max_latency_ns_.load(
        std::memory_order_relaxed
    );
}

double LockFreeWorkerPool::average_latency_us() const {
    const auto count = processed();

    if (count == 0) {
        return 0.0;
    }

    return static_cast<double>(
        total_latency_ns()
    )
    /
    static_cast<double>(count)
    /
    1000.0;
}

double LockFreeWorkerPool::checksum() const {
    return checksum_.load(
        std::memory_order_relaxed
    );
}

std::size_t LockFreeWorkerPool::queue_capacity() const {
    return queue_.capacity();
}

std::size_t LockFreeWorkerPool::worker_count() const {
    return worker_count_;
}


std::vector<std::uint64_t>
LockFreeWorkerPool::collect_latencies() const {

    std::vector<std::uint64_t> result;

    std::lock_guard<std::mutex> lock(
        latency_mutex_
    );

    result.reserve(
        static_cast<std::size_t>(processed())
    );

    for (const auto& worker_latencies : latency_samples_) {

        result.insert(
            result.end(),
            worker_latencies.begin(),
            worker_latencies.end()
        );
    }

    return result;
}
} // namespace nexusflow

