#include <chrono>
#include <cstdint>
#include <iostream>
#include <vector>

#include "core/event/event.hpp"
#include "core/scheduler/adaptive_scheduler.hpp"

using namespace nexusflow;

static const char* priority_name(
    EventPriority priority
) {
    switch (priority) {
        case EventPriority::LOW:
            return "LOW";

        case EventPriority::NORMAL:
            return "NORMAL";

        case EventPriority::HIGH:
            return "HIGH";

        case EventPriority::CRITICAL:
            return "CRITICAL";
    }

    return "UNKNOWN";
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

static void run_case(
    EventPriority priority,
    std::size_t queue_depth,
    double arrival_rate,
    std::uint64_t sla_us
) {
    SchedulerMetrics metrics;

    metrics.arrival_rate_eps =
        arrival_rate;

    metrics.queue_depth =
        queue_depth;

    metrics.cpu_utilization =
        0.50;

    metrics.remaining_sla_us =
        sla_us;

    metrics.highest_priority =
        priority;

    AdaptiveScheduler scheduler;

    const SchedulingDecision decision =
        scheduler.decide(metrics);

    std::cout
        << "Priority: "
        << priority_name(priority)
        << "\n";

    std::cout
        << "Queue depth: "
        << queue_depth
        << "\n";

    std::cout
        << "Arrival rate: "
        << arrival_rate
        << " events/sec\n";

    std::cout
        << "SLA remaining: "
        << sla_us
        << " us\n";

    std::cout
        << "Mode: "
        << mode_name(decision.mode)
        << "\n";

    std::cout
        << "Batch size: "
        << decision.batch_size
        << "\n";

    std::cout
        << "Target workers: "
        << decision.target_workers
        << "\n";

    std::cout
        << "SLA bypass: "
        << (decision.sla_bypass ? "YES" : "NO")
        << "\n";

    std::cout
        << "--------------------------------------------\n";
}

int main() {
    std::cout
        << "============================================\n";

    std::cout
        << "NexusFlow Priority Scheduling Benchmark\n";

    std::cout
        << "============================================\n\n";

    std::cout
        << "[LOW PRIORITY]\n";

    run_case(
        EventPriority::LOW,
        4,
        500.0,
        5000
    );

    std::cout
        << "\n[NORMAL PRIORITY]\n";

    run_case(
        EventPriority::NORMAL,
        128,
        3000.0,
        5000
    );

    std::cout
        << "\n[HIGH PRIORITY]\n";

    run_case(
        EventPriority::HIGH,
        128,
        3000.0,
        5000
    );

    std::cout
        << "\n[CRITICAL PRIORITY]\n";

    run_case(
        EventPriority::CRITICAL,
        2048,
        10000.0,
        5000
    );

    std::cout
        << "\n[SLA PRESSURE]\n";

    run_case(
        EventPriority::NORMAL,
        512,
        5000.0,
        300
    );

    std::cout
        << "\nPriority scheduling benchmark completed.\n";

    return 0;
}
