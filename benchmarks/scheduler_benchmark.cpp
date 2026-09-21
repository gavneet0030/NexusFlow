#include "core/scheduler/adaptive_scheduler.hpp"
#include "core/scheduler/fixed_scheduler.hpp"

#include <iostream>
#include <string>
#include <vector>

using namespace nexusflow;

static std::string mode_to_string(ProcessingMode mode) {
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
    FixedScheduler fixed_scheduler;
    AdaptiveScheduler adaptive_scheduler;

    std::vector<SchedulerMetrics> workloads = {
        {100.0, 2, 0.20, 5000},
        {5000.0, 32, 0.40, 5000},
        {20000.0, 128, 0.60, 3000},
        {50000.0, 512, 0.75, 2000},
        {100000.0, 2048, 0.90, 1000},
        {100000.0, 2048, 0.95, 300}
    };

    std::cout << "NexusFlow Scheduler Benchmark\n";
    std::cout << "=============================\n\n";

    for (std::size_t i = 0; i < workloads.size(); ++i) {
        const auto& metrics = workloads[i];

        const auto fixed = fixed_scheduler.decide(metrics);
        const auto adaptive = adaptive_scheduler.decide(metrics);

        std::cout << "Workload " << (i + 1) << "\n";
        std::cout << "Arrival rate: " << metrics.arrival_rate_eps << " events/sec\n";
        std::cout << "Queue depth: " << metrics.queue_depth << "\n";
        std::cout << "CPU utilization: " << metrics.cpu_utilization << "\n";
        std::cout << "SLA remaining: " << metrics.remaining_sla_us << " us\n";

        std::cout << "Fixed:    "
                  << mode_to_string(fixed.mode)
                  << ", batch=" << fixed.batch_size
                  << ", workers=" << fixed.target_workers
                  << "\n";

        std::cout << "Adaptive: "
                  << mode_to_string(adaptive.mode)
                  << ", batch=" << adaptive.batch_size
                  << ", workers=" << adaptive.target_workers
                  << "\n\n";
    }

    std::cout << "Scheduler benchmark completed successfully.\n";

    return 0;
}
