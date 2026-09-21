#include <chrono>
#include <cstdint>
#include <iostream>

#include "core/processor/integrated_adaptive_pipeline.hpp"

using namespace nexusflow;

static Event create_event(std::uint64_t id) {
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
    event.value = static_cast<double>(id % 1000);
    event.source = "adaptive-integration";
    event.type = "synthetic";

    return event;
}

static const char* mode_name(
    ProcessingMode mode
) {
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
    constexpr std::uint64_t event_count = 50000;

    std::cout
        << "============================================\n";

    std::cout
        << "NexusFlow Adaptive Worker Integration\n";

    std::cout
        << "============================================\n\n";

    IntegratedAdaptivePipeline pipeline(
        4096,
        16
    );

    pipeline.start();

    const auto start =
        std::chrono::steady_clock::now();

    for (std::uint64_t id = 0;
         id < event_count;
         ++id) {

        pipeline.submit(
            create_event(id)
        );

        if ((id + 1) % 256 == 0) {
            pipeline.update_scheduler();
        }
    }

    pipeline.drain();

    const auto end =
        std::chrono::steady_clock::now();

    const double elapsed =
        std::chrono::duration<double>(
            end - start
        ).count();

    const PipelineMetrics metrics =
        pipeline.metrics();

    const SchedulingDecision decision =
        pipeline.current_decision();

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
        << metrics.queue_depth
        << "\n";

    std::cout
        << "Active workers: "
        << metrics.active_workers
        << "\n";

    std::cout
        << "Scheduler mode: "
        << mode_name(decision.mode)
        << "\n";

    std::cout
        << "Scheduler batch size: "
        << decision.batch_size
        << "\n";

    std::cout
        << "Scheduler target workers: "
        << decision.target_workers
        << "\n";

    std::cout
        << "Elapsed seconds: "
        << elapsed
        << "\n";

    if (elapsed > 0.0) {
        std::cout
            << "Processed throughput: "
            << static_cast<double>(
                metrics.processed
            ) / elapsed
            << " events/sec\n";
    }

    std::cout << "\n";

    if (
        metrics.processed ==
        metrics.accepted
    ) {
        std::cout
            << "Pipeline integrity: PASS\n";
    }
    else {
        std::cout
            << "Pipeline integrity: CHECK REQUIRED\n";
    }

    pipeline.shutdown();

    std::cout
        << "\nAdaptive worker integration completed.\n";

    return 0;
}

