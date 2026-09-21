#pragma once

#include "core/event/event.hpp"
#include "core/processor/event_processor.hpp"
#include "core/queue/bounded_mpmc_queue.hpp"

#include <atomic>
#include <cstdint>
#include <thread>
#include <vector>

namespace nexusflow {

class Worker {

public:

    Worker(
        BoundedMPMCQueue<Event>& queue,
        std::atomic<std::uint64_t>& processed,
        std::atomic<std::uint64_t>& total_latency_ns,
        std::atomic<std::uint64_t>& max_latency_ns,
        std::atomic<double>& checksum
    );

    void start();

    void join();

    void request_failure();

    void reset_failure();

    bool failure_requested() const;

    const std::vector<std::uint64_t>& latencies() const;

private:

    void run();

    BoundedMPMCQueue<Event>& queue_;

    std::atomic<std::uint64_t>& processed_;

    std::atomic<std::uint64_t>& total_latency_ns_;

    std::atomic<std::uint64_t>& max_latency_ns_;

    std::atomic<double>& checksum_;

    EventProcessor processor_;

    std::thread thread_;

    std::vector<std::uint64_t> latencies_;

    std::atomic<bool> failure_requested_{false};
};

} // namespace nexusflow
