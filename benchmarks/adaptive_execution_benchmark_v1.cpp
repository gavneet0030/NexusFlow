#include "core/event/event.hpp"
#include "core/processor/adaptive_execution_engine.hpp"
#include "core/scheduler/adaptive_scheduler.hpp"
#include "core/scheduler/fixed_scheduler.hpp"

#include <chrono>
#include <iomanip>
#include <iostream>
#include <vector>

using namespace nexusflow;

static std::uint64_t now_ns() {
    return static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now().time_since_epoch()
        ).count()
    );
}

static double to_microseconds(
    std::uint64_t nanoseconds
) {
    return static_cast<double>(nanoseconds) / 1000.0;
}

static void print_result(
    const char* name,
    const ExecutionMetrics& metrics,
    double elapsed_seconds
) {
    const double throughput =
        elapsed_seconds > 0.0
            ? static_cast<double>(metrics.events_processed) /
              elapsed_seconds
            : 0.0;

    const double average_latency =
        metrics.events_processed > 0
            ? to_microseconds(
                  metrics.total_latency_ns /
                  metrics.events_processed
              )
            : 0.0;

    std::cout << name << "\n";
    std::cout << "  Events: " << metrics.events_processed << "\n";
    std::cout << "  Throughput: "
              << std::fixed << std::setprecision(2)
              << throughput
              << " events/sec\n";

    std::cout << "  Average latency: "
              << average_latency
              << " us\n";

    std::cout << "  Max latency: "
              << to_microseconds(metrics.max_latency_ns)
              << " us\n";

    std::cout << "  Checksum: "
              << metrics.checksum
              << "\n\n";
}

int main() {
    constexpr std::size_t event_count = 100000;

    std::vector<Event> events;
    events.reserve(event_count);

    for (std::size_t i = 0; i < event_count; ++i) {
        Event event;

        event.id = static_cast<std::uint64_t>(i + 1);
        event.timestamp_ns = now_ns();

        event.priority =
            (i % 100 == 0)
                ? EventPriority::HIGH
                : EventPriority::NORMAL;

        event.value =
            static_cast<double>((i % 1000) + 1);

        event.source = "synthetic";
        event.type = "transaction";

        events.push_back(std::move(event));
    }

    SchedulerMetrics scheduler_metrics;

    scheduler_metrics.arrival_rate_eps = 50000.0;
    scheduler_metrics.queue_depth = event_count;
    scheduler_metrics.cpu_utilization = 0.70;
    scheduler_metrics.remaining_sla_us = 5000;

    FixedScheduler fixed_scheduler(16, 4);
    AdaptiveScheduler adaptive_scheduler(1, 16);

    const SchedulingDecision fixed_decision =
        fixed_scheduler.decide(scheduler_metrics);

    const SchedulingDecision adaptive_decision =
        adaptive_scheduler.decide(scheduler_metrics);

    AdaptiveExecutionEngine engine(16);

    std::cout << "NexusFlow Adaptive Execution Benchmark\n";
    std::cout << "=======================================\n\n";

    std::cout << "Fixed scheduler decision:\n";
    std::cout << "  Batch size: "
              << fixed_decision.batch_size
              << "\n";

    std::cout << "  Workers: "
              << fixed_decision.target_workers
              << "\n\n";

    std::cout << "Adaptive scheduler decision:\n";
    std::cout << "  Batch size: "
              << adaptive_decision.batch_size
              << "\n";

    std::cout << "  Workers: "
              << adaptive_decision.target_workers
              << "\n\n";

    const auto fixed_start =
        std::chrono::steady_clock::now();

    const ExecutionMetrics fixed_metrics =
        engine.execute(
            events,
            fixed_decision
        );

    const auto fixed_end =
        std::chrono::steady_clock::now();

    const double fixed_elapsed =
        std::chrono::duration<double>(
            fixed_end - fixed_start
        ).count();

    const auto adaptive_start =
        std::chrono::steady_clock::now();

    const ExecutionMetrics adaptive_metrics =
        engine.execute(
            events,
            adaptive_decision
        );

    const auto adaptive_end =
        std::chrono::steady_clock::now();

    const double adaptive_elapsed =
        std::chrono::duration<double>(
            adaptive_end - adaptive_start
        ).count();

    print_result(
        "Fixed execution",
        fixed_metrics,
        fixed_elapsed
    );

    print_result(
        "Adaptive execution",
        adaptive_metrics,
        adaptive_elapsed
    );

    std::cout << "Adaptive execution benchmark completed.\n";

    return 0;
}
