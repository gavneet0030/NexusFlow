#include "core/processor/adaptive_event_pipeline.hpp"

#include <chrono>
#include <cstdint>
#include <iostream>
#include <thread>
#include <vector>

using namespace nexusflow;

static Event make_event(
    std::uint64_t id
) {
    Event event;

    event.id = id;
    event.timestamp_ns =
        static_cast<std::uint64_t>(
            std::chrono::duration_cast<
                std::chrono::nanoseconds
            >(
                std::chrono::steady_clock::now()
                    .time_since_epoch()
            ).count()
        );

    event.priority = EventPriority::NORMAL;
    event.value = static_cast<double>(id);
    event.source = "backpressure-benchmark";
    event.type = "synthetic";

    return event;
}

int main() {
    constexpr std::size_t queue_capacity = 256;
    constexpr std::size_t total_events = 100000;

    AdaptiveEventPipeline pipeline(
        queue_capacity,
        16
    );

    pipeline.start();

    std::cout << "============================================\n";
    std::cout << "NexusFlow Backpressure Benchmark\n";
    std::cout << "============================================\n";

    std::cout
        << "Queue capacity: "
        << queue_capacity
        << "\n";

    std::cout
        << "Events submitted: "
        << total_events
        << "\n\n";

    const auto start =
        std::chrono::steady_clock::now();

    std::size_t processed_total = 0;

    for (std::size_t i = 0;
         i < total_events;
         ++i) {

        pipeline.submit(
            make_event(i)
        );

        if ((i % 8) == 0) {
            processed_total +=
                pipeline.process();
        }
    }

    for (;;) {
        const std::size_t processed =
            pipeline.process();

        processed_total += processed;

        if (pipeline.queue_depth() == 0) {
            break;
        }
    }

    const auto end =
        std::chrono::steady_clock::now();

    const double elapsed_seconds =
        std::chrono::duration<double>(
            end - start
        ).count();

    const PipelineMetrics metrics =
        pipeline.metrics();

    std::cout << "\n";
    std::cout << "Results\n";
    std::cout << "--------------------------------------------\n";

    std::cout
        << "Submitted: "
        << metrics.submitted
        << "\n";

    std::cout
        << "Accepted: "
        << metrics.accepted
        << "\n";

    std::cout
        << "Throttled: "
        << metrics.throttled
        << "\n";

    std::cout
        << "Rejected: "
        << metrics.rejected
        << "\n";

    std::cout
        << "Processed: "
        << metrics.processed
        << "\n";

    std::cout
        << "Final queue depth: "
        << pipeline.queue_depth()
        << "\n";

    std::cout
        << "Elapsed seconds: "
        << elapsed_seconds
        << "\n";

    if (elapsed_seconds > 0.0) {
        std::cout
            << "Submission rate: "
            << static_cast<double>(
                   metrics.submitted
               ) / elapsed_seconds
            << " events/sec\n";
    }

    std::cout << "\n";

    if (metrics.rejected > 0) {
        std::cout
            << "Backpressure protection: ACTIVE\n";
    }
    else {
        std::cout
            << "Backpressure protection: NOT TRIGGERED\n";
    }

    pipeline.shutdown();

    return 0;
}
