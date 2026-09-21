#include "core/event/latency.hpp"
#include "core/event/latency_statistics.hpp"

#include <chrono>
#include <iostream>
#include <thread>
#include <vector>

int main() {
    using namespace nexusflow;

    constexpr std::size_t sample_count = 1000;

    std::vector<std::uint64_t> queue_wait;
    std::vector<std::uint64_t> processing;
    std::vector<std::uint64_t> end_to_end;

    queue_wait.reserve(sample_count);
    processing.reserve(sample_count);
    end_to_end.reserve(sample_count);

    for (std::size_t i = 0; i < sample_count; ++i) {
        LatencyBreakdown latency;

        latency.event_created_ns =
            steady_clock_ns();

        std::this_thread::sleep_for(
            std::chrono::microseconds(5)
        );

        latency.accepted_ns =
            steady_clock_ns();

        std::this_thread::sleep_for(
            std::chrono::microseconds(10)
        );

        latency.processing_start_ns =
            steady_clock_ns();

        std::this_thread::sleep_for(
            std::chrono::microseconds(5)
        );

        latency.processing_end_ns =
            steady_clock_ns();

        latency.completed_ns =
            steady_clock_ns();

        queue_wait.push_back(
            latency.queue_wait_us()
        );

        processing.push_back(
            latency.processing_us()
        );

        end_to_end.push_back(
            latency.end_to_end_us()
        );
    }

    const auto queue_stats =
        calculate_latency_percentiles(queue_wait);

    const auto processing_stats =
        calculate_latency_percentiles(processing);

    const auto end_to_end_stats =
        calculate_latency_percentiles(end_to_end);

    std::cout << "\n";
    std::cout << "========================================\n";
    std::cout << "NEXUSFLOW LATENCY INSTRUMENTATION TEST\n";
    std::cout << "========================================\n\n";

    std::cout << "Samples: "
              << sample_count
              << "\n\n";

    std::cout << "Queue Wait Latency\n";
    std::cout << "  P50:   "
              << queue_stats.p50_us
              << " us\n";
    std::cout << "  P95:   "
              << queue_stats.p95_us
              << " us\n";
    std::cout << "  P99:   "
              << queue_stats.p99_us
              << " us\n";
    std::cout << "  P99.9: "
              << queue_stats.p999_us
              << " us\n";
    std::cout << "  Max:   "
              << queue_stats.max_us
              << " us\n\n";

    std::cout << "Processing Latency\n";
    std::cout << "  P50:   "
              << processing_stats.p50_us
              << " us\n";
    std::cout << "  P95:   "
              << processing_stats.p95_us
              << " us\n";
    std::cout << "  P99:   "
              << processing_stats.p99_us
              << " us\n";
    std::cout << "  P99.9: "
              << processing_stats.p999_us
              << " us\n";
    std::cout << "  Max:   "
              << processing_stats.max_us
              << " us\n\n";

    std::cout << "End-to-End Latency\n";
    std::cout << "  P50:   "
              << end_to_end_stats.p50_us
              << " us\n";
    std::cout << "  P95:   "
              << end_to_end_stats.p95_us
              << " us\n";
    std::cout << "  P99:   "
              << end_to_end_stats.p99_us
              << " us\n";
    std::cout << "  P99.9: "
              << end_to_end_stats.p999_us
              << " us\n";
    std::cout << "  Max:   "
              << end_to_end_stats.max_us
              << " us\n\n";

    const bool pass =
        queue_wait.size() == sample_count &&
        processing.size() == sample_count &&
        end_to_end.size() == sample_count;

    std::cout << "========================================\n";
    std::cout << "Integrity: "
              << (pass ? "PASS" : "FAIL")
              << "\n";
    std::cout << "========================================\n";

    return pass ? 0 : 1;
}
