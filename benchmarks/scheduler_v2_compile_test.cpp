#include "core/scheduler/scheduler_v2.hpp"

#include <iostream>

int main() {
    nexusflow::SchedulerV2 scheduler;

    nexusflow::SchedulerMetrics metrics;

    metrics.arrival_rate_eps = 10000.0;
    metrics.queue_depth = 64;
    metrics.cpu_utilization = 0.80;
    metrics.remaining_sla_us = 5000;
    metrics.highest_priority = nexusflow::EventPriority::NORMAL;

    const auto decision = scheduler.decide(metrics);

    std::cout << "NexusFlow Scheduler V2 compile test" << std::endl;
    std::cout << "Target workers: "
              << decision.target_workers
              << std::endl;

    std::cout << "Batch size: "
              << decision.batch_size
              << std::endl;

    std::cout << "SLA bypass: "
              << (decision.sla_bypass ? "true" : "false")
              << std::endl;

    return 0;
}
