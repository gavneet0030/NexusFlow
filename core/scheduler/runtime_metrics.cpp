#include "core/scheduler/runtime_metrics.hpp"

#include <algorithm>
#include <thread>

namespace nexusflow {

RuntimeMetrics::RuntimeMetrics()
    : window_start_(Clock::now()) {
}

void RuntimeMetrics::record_event_arrival() {
    ++arrivals_in_window_;

    const auto now = Clock::now();

    const auto elapsed =
        std::chrono::duration_cast<
            std::chrono::milliseconds
        >(now - window_start_).count();

    if (elapsed >= 1000) {
        arrivals_in_window_ = 0;
        window_start_ = now;
    }
}

void RuntimeMetrics::record_event_processed(
    std::uint64_t latency_ns
) {
    ++processed_events_;
    total_latency_ns_ += latency_ns;
}

void RuntimeMetrics::set_queue_depth(
    std::size_t queue_depth
) {
    queue_depth_ = queue_depth;
}

double RuntimeMetrics::arrival_rate_eps() const {
    const auto now = Clock::now();

    const double elapsed_seconds =
        std::chrono::duration<double>(
            now - window_start_
        ).count();

    if (elapsed_seconds <= 0.0) {
        return 0.0;
    }

    return static_cast<double>(
        arrivals_in_window_
    ) / elapsed_seconds;
}

std::size_t RuntimeMetrics::queue_depth() const {
    return queue_depth_;
}

double RuntimeMetrics::cpu_utilization() const {
    const unsigned int hardware_threads =
        std::max(
            1u,
            std::thread::hardware_concurrency()
        );

    const double estimated_workers =
        static_cast<double>(
            std::min(
                queue_depth_ / 64 + 1,
                static_cast<std::size_t>(
                    hardware_threads
                )
            )
        );

    return std::min(
        1.0,
        estimated_workers /
        static_cast<double>(hardware_threads)
    );
}

std::uint64_t RuntimeMetrics::average_latency_ns() const {
    if (processed_events_ == 0) {
        return 0;
    }

    return total_latency_ns_ /
           processed_events_;
}

} // namespace nexusflow
