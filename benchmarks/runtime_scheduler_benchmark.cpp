#include "core/scheduler/runtime_scheduler.hpp"

#include <chrono>
#include <iostream>
#include <thread>

using namespace nexusflow;

static const char* mode_to_string(
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

static void print_decision(
    const char* label,
    const SchedulingDecision& decision,
    const SchedulerMetrics& metrics
) {
    std::cout
        << label
        << " | arrival_rate="
        << metrics.arrival_rate_eps
        << " eps"
        << " | queue="
        << metrics.queue_depth
        << " | mode="
        << mode_to_string(decision.mode)
        << " | batch="
        << decision.batch_size
        << " | workers="
        << decision.target_workers
        << " | sla_bypass="
        << (decision.sla_bypass ? "true" : "false")
        << "\n";
}

int main() {
    RuntimeScheduler scheduler(16);

    std::cout
        << "NexusFlow Runtime Scheduler Benchmark\n";

    std::cout
        << "=====================================\n\n";

    std::cout
        << "Phase 1: low load\n";

    for (int i = 0; i < 5; ++i) {
        scheduler.on_event_arrival();

        scheduler.update_queue_depth(2);

        std::this_thread::sleep_for(
            std::chrono::milliseconds(100)
        );
    }

    auto decision = scheduler.decide(2);

    print_decision(
        "LOW LOAD",
        decision,
        scheduler.metrics()
    );

    std::cout
        << "\nPhase 2: medium load\n";

    for (int i = 0; i < 50; ++i) {
        scheduler.on_event_arrival();
        scheduler.update_queue_depth(64);
    }

    decision = scheduler.decide(64);

    print_decision(
        "MEDIUM LOAD",
        decision,
        scheduler.metrics()
    );

    std::cout
        << "\nPhase 3: high load\n";

    for (int i = 0; i < 500; ++i) {
        scheduler.on_event_arrival();
        scheduler.update_queue_depth(512);
    }

    decision = scheduler.decide(512);

    print_decision(
        "HIGH LOAD",
        decision,
        scheduler.metrics()
    );

    std::cout
        << "\nPhase 4: SLA pressure\n";

    scheduler.update_queue_depth(2048);

    SchedulerMetrics sla_metrics =
        scheduler.metrics();

    sla_metrics.remaining_sla_us = 300;

    decision = scheduler.decide(
        sla_metrics
    );

    print_decision(
        "SLA PRESSURE",
        decision,
        sla_metrics
    );

    std::cout
        << "\nRuntime scheduler benchmark completed successfully.\n";

    return 0;
}
