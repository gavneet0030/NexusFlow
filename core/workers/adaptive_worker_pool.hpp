#pragma once

#include "core/event/event.hpp"
#include "core/processor/event_processor.hpp"
#include "core/queue/bounded_mpmc_queue.hpp"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

namespace nexusflow {

struct AdaptiveWorkerResult {
    std::uint64_t event_id{0};
    std::uint64_t latency_ns{0};
    double result{0.0};
};

class AdaptiveWorkerPool {
public:

    using Processor =
        std::function<void(const Event&)>;

    AdaptiveWorkerPool(
        BoundedMPMCQueue<Event>& queue,
        std::size_t max_workers,
        Processor processor
    );

    ~AdaptiveWorkerPool();

    AdaptiveWorkerPool(
        const AdaptiveWorkerPool&
    ) = delete;

    AdaptiveWorkerPool& operator=(
        const AdaptiveWorkerPool&
    ) = delete;

    void start();

    void set_active_workers(
        std::size_t worker_count
    );

    void set_batch_size(
        std::size_t batch_size
    );

    std::size_t batch_size() const;

    std::size_t active_workers() const;

    std::size_t max_workers() const;

    std::uint64_t processed_events() const;

    std::uint64_t idle_cycles() const;

    bool running() const;

    void wait_until_processed(
        std::uint64_t expected_count
    );

    const std::vector<AdaptiveWorkerResult>&
    results() const;

    void reset_metrics();

    void stop();

private:

    void worker_loop(
        std::size_t worker_id
    );

    BoundedMPMCQueue<Event>& queue_;

    std::size_t max_workers_;

    Processor processor_;

    EventProcessor event_processor_;

    std::vector<std::thread> workers_;

    std::atomic<std::size_t>
        active_workers_{1};

    std::atomic<std::size_t>
        batch_size_{1};

    std::atomic<std::uint64_t>
        processed_events_{0};

    std::atomic<std::uint64_t>
        idle_cycles_{0};

    std::atomic<bool>
        running_{false};

    std::atomic<bool>
        started_{false};

    mutable std::mutex results_mutex_;

    std::vector<AdaptiveWorkerResult>
        results_;
};

} // namespace nexusflow


