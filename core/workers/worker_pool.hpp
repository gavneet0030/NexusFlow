#pragma once

#include "core/event/event.hpp"
#include "core/queue/bounded_mpmc_queue.hpp"
#include "core/workers/worker.hpp"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

namespace nexusflow {

class WorkerPool {

public:

    WorkerPool(
        std::size_t worker_count,
        std::size_t queue_capacity
    );

    ~WorkerPool();

    WorkerPool(const WorkerPool&) = delete;

    WorkerPool& operator=(const WorkerPool&) = delete;

    void start();

    bool submit(Event event);

    void shutdown();

    bool inject_worker_failure(
        std::size_t worker_id
    );

    bool recover_worker(
        std::size_t worker_id
    );

    bool worker_failure_requested(
        std::size_t worker_id
    ) const;

    std::uint64_t processed() const;

    std::uint64_t total_latency_ns() const;

    std::uint64_t max_latency_ns() const;

    double average_latency_us() const;

    double checksum() const;

    std::size_t queue_size() const;

    std::size_t worker_count() const;

    std::vector<std::uint64_t> collect_latencies() const;

private:

    std::unique_ptr<Worker> create_worker();

    BoundedMPMCQueue<Event> queue_;

    std::vector<std::unique_ptr<Worker>> workers_;

    std::atomic<std::uint64_t> processed_{0};

    std::atomic<std::uint64_t> total_latency_ns_{0};

    std::atomic<std::uint64_t> max_latency_ns_{0};

    std::atomic<double> checksum_{0.0};

    bool started_{false};

    bool shutdown_{false};
};

} // namespace nexusflow
