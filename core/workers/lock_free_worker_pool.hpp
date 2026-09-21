#pragma once

#include "core/event/event.hpp"
#include "core/processor/event_processor.hpp"
#include "core/queue/lock_free_mpmc_queue.hpp"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <thread>
#include <vector>
#include <mutex>

namespace nexusflow {

class LockFreeWorkerPool {
public:
    LockFreeWorkerPool(
        std::size_t worker_count,
        std::size_t queue_capacity
    );

    ~LockFreeWorkerPool();

    LockFreeWorkerPool(const LockFreeWorkerPool&) = delete;
    LockFreeWorkerPool& operator=(const LockFreeWorkerPool&) = delete;

    void start();

    bool submit(Event event);

    void shutdown();

    std::uint64_t processed() const;

    std::uint64_t total_latency_ns() const;

    std::uint64_t max_latency_ns() const;

    double average_latency_us() const;

    double checksum() const;

    std::size_t queue_capacity() const;

    std::size_t worker_count() const;

    std::vector<std::uint64_t> collect_latencies() const;

private:
    void worker_loop(std::size_t worker_index);

    LockFreeMPMCQueue<Event> queue_;

    std::vector<std::thread> workers_;

    std::vector<std::vector<std::uint64_t>> latency_samples_;
    mutable std::mutex latency_mutex_;

    std::size_t worker_count_;

    std::atomic<bool> started_{false};
    std::atomic<bool> shutdown_{false};

    std::atomic<std::uint64_t> processed_{0};
    std::atomic<std::uint64_t> total_latency_ns_{0};
    std::atomic<std::uint64_t> max_latency_ns_{0};

    std::atomic<double> checksum_{0.0};
};

} // namespace nexusflow


