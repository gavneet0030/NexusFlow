#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>

namespace nexusflow {

class RuntimeMetrics {
public:
    RuntimeMetrics();

    void record_event_arrival();

    void record_event_processed(
        std::uint64_t latency_ns
    );

    void set_queue_depth(
        std::size_t queue_depth
    );

    double arrival_rate_eps() const;

    std::size_t queue_depth() const;

    double cpu_utilization() const;

    std::uint64_t average_latency_ns() const;

private:
    using Clock = std::chrono::steady_clock;

    Clock::time_point window_start_;

    std::uint64_t arrivals_in_window_{0};
    std::uint64_t processed_events_{0};
    std::uint64_t total_latency_ns_{0};

    std::size_t queue_depth_{0};
};

} // namespace nexusflow
