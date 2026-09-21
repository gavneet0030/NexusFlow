#include <algorithm>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <vector>

#include "core/event/event.hpp"
#include "core/processor/adaptive_event_pipeline.hpp"

using namespace nexusflow;

static Event make_event(uint64_t id) {
    Event event;
    event.id = id;
    event.timestamp_ns =
        static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(
                std::chrono::steady_clock::now().time_since_epoch()
            ).count()
        );
    event.priority = EventPriority::NORMAL;
    event.value = static_cast<double>(id % 1000);
    event.source = "adaptive-benchmark";
    event.type = "synthetic";

    return event;
}

static const char* mode_name(ProcessingMode mode) {
    switch (mode) {
        case ProcessingMode::SINGLE:
            return "SINGLE";

        case ProcessingMode::MICRO_BATCH:
            return "MICRO_BATCH";

        case ProcessingMode::PARALLEL:
            return "PARALLEL";
    }

    return "UNKNOWN";
}

int main() {
    constexpr size_t event_count = 10000;

    std::cout << "============================================\n";
    std::cout << "NexusFlow Adaptive Event Pipeline Benchmark\n";
    std::cout << "============================================\n\n";

    AdaptiveEventPipeline pipeline(4096, 16);

    pipeline.start();

    const auto start_time = std::chrono::steady_clock::now();

    for (size_t i = 0; i < event_count; ++i) {
        pipeline.submit(make_event(static_cast<uint64_t>(i)));

        if ((i + 1) % 32 == 0) {
            pipeline.process();
        }
    }

    while (pipeline.queue_depth() > 0) {
        pipeline.process();
    }

    const auto end_time = std::chrono::steady_clock::now();

    pipeline.shutdown();

    const double elapsed_seconds =
        std::chrono::duration<double>(
            end_time - start_time
        ).count();

    const PipelineMetrics metrics = pipeline.metrics();
    const SchedulingDecision decision = pipeline.current_decision();

    std::cout << "Submitted: "
              << metrics.submitted
              << "\n";

    std::cout << "Accepted: "
              << metrics.accepted
              << "\n";

    std::cout << "Throttled: "
              << metrics.throttled
              << "\n";

    std::cout << "Rejected: "
              << metrics.rejected
              << "\n";

    std::cout << "Processed: "
              << metrics.processed
              << "\n";

    std::cout << "Final queue depth: "
              << pipeline.queue_depth()
              << "\n";

    std::cout << "Final processing mode: "
              << mode_name(decision.mode)
              << "\n";

    std::cout << "Final batch size: "
              << decision.batch_size
              << "\n";

    std::cout << "Final target workers: "
              << decision.target_workers
              << "\n";

    std::cout << "Elapsed seconds: "
              << elapsed_seconds
              << "\n";

    if (elapsed_seconds > 0.0) {
        const double throughput =
            static_cast<double>(metrics.processed) /
            elapsed_seconds;

        std::cout << "Processing throughput: "
                  << throughput
                  << " events/sec\n";
    }

    std::cout << "\n";

    if (metrics.processed == metrics.accepted) {
        std::cout << "Pipeline integrity: PASS\n";
    } else {
        std::cout << "Pipeline integrity: CHECK REQUIRED\n";
    }

    std::cout << "Adaptive pipeline benchmark completed.\n";

    return 0;
}
