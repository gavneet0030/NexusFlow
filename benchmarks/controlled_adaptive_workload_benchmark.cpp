#include <algorithm>
#include "core/processor/integrated_adaptive_pipeline.hpp"

#include <chrono>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

namespace {

using nexusflow::Event;
using nexusflow::EventPriority;
using nexusflow::IntegratedAdaptivePipeline;
using nexusflow::ProcessingMode;
using nexusflow::SchedulingDecision;

const char* mode_name(ProcessingMode mode) {
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

Event make_event(std::uint64_t id) {
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
    event.source = "controlled_workload";
    event.type = "benchmark";

    return event;
}

void print_decision(
    const std::string& scenario,
    const SchedulingDecision& decision
) {
    std::cout
        << std::left
        << std::setw(12)
        << scenario
        << std::setw(14)
        << mode_name(decision.mode)
        << std::setw(12)
        << decision.batch_size
        << std::setw(12)
        << decision.target_workers
        << '\n';
}

void run_scenario(
    const std::string& name,
    std::size_t event_count,
    std::chrono::microseconds delay
) {
    IntegratedAdaptivePipeline pipeline(
        4096,
        16
    );

    pipeline.start();

    std::cout
        << "\nScenario: "
        << name
        << '\n';

    std::cout
        << "Events: "
        << event_count
        << '\n';

    std::cout
        << "Delay between events: "
        << delay.count()
        << " us\n";

    std::cout
        << "\n"
        << std::left
        << std::setw(12)
        << "Scenario"
        << std::setw(14)
        << "Mode"
        << std::setw(12)
        << "Batch"
        << std::setw(12)
        << "Workers"
        << '\n';

    std::cout
        << "----------------------------------------------\n";

    std::uint64_t accepted = 0;

    for (std::size_t i = 0; i < event_count; ++i) {
        if (pipeline.submit(make_event(i))) {
            ++accepted;
        }

        if ((i + 1) % 64 == 0) {
            pipeline.update_scheduler();

            print_decision(
                name,
                pipeline.current_decision()
            );
        }

        if (delay.count() > 0) {
            std::this_thread::sleep_for(delay);
        }
    }

    pipeline.drain();

    const auto metrics =
        pipeline.metrics();

    std::cout
        << "\nAccepted: "
        << accepted
        << '\n';

    std::cout
        << "Processed: "
        << metrics.processed
        << '\n';

    std::cout
        << "Rejected: "
        << metrics.rejected
        << '\n';

    std::cout
        << "Throttled: "
        << metrics.throttled
        << '\n';

    std::cout
        << "Final queue depth: "
        << metrics.queue_depth
        << '\n';

    std::cout
        << "Final mode: "
        << mode_name(
            pipeline.current_decision().mode
        )
        << '\n';

    std::cout
        << "Final workers: "
        << metrics.active_workers
        << '\n';

    std::cout
        << "Average latency: "
        << std::fixed
        << std::setprecision(3)
        << metrics.average_latency_us
        << " us\n";

    std::cout
        << "Integrity: "
        << (
            metrics.processed == accepted
                ? "PASS"
                : "FAIL"
        )
        << '\n';

    pipeline.shutdown();
}

} // namespace

int main() {
    std::cout
        << "============================================\n"
        << "NexusFlow Controlled Adaptive Workload\n"
        << "============================================\n";

    run_scenario(
        "LOW",
        256,
        std::chrono::microseconds(2000)
    );

    run_scenario(
        "MEDIUM",
        4096,
        std::chrono::microseconds(200)
    );

    run_scenario(
        "HIGH",
        20000,
        std::chrono::microseconds(0)
    );

    std::cout
        << "\n============================================\n"
        << "Controlled workload experiment completed.\n"
        << "============================================\n";

    return 0;
}


