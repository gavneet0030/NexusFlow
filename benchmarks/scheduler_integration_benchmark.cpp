#include "core/scheduler/adaptive_scheduler.hpp"
#include "core/scheduler/fixed_scheduler.hpp"
#include "core/scheduler/scheduler_controller.hpp"

#include <iostream>
#include <memory>
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

struct Workload {
    double arrival_rate;
    std::size_t queue_depth;
    double cpu;
    std::uint64_t sla_us;
};

int main() {
    SchedulerController fixed(
        std::make_unique<FixedScheduler>(),
        16
    );

    SchedulerController adaptive(
        std::make_unique<AdaptiveScheduler>(1, 16),
        16
    );

    const std::vector<Workload> workloads = {
        {100.0, 2, 0.20, 5000},
        {5000.0, 32, 0.40, 5000},
        {20000.0, 128, 0.60, 3000},
        {50000.0, 512, 0.75, 2000},
        {100000.0, 2048, 0.90, 1000},
        {100000.0, 2048, 0.95, 300}
    };

    std::cout << "NexusFlow Scheduler Integration Benchmark\n";
    std::cout << "=========================================\n\n";

    for (std::size_t i = 0; i < workloads.size(); ++i) {
        const auto& workload = workloads[i];

        const auto fixed_decision = fixed.update(
            workload.arrival_rate,
            workload.queue_depth,
            workload.cpu,
            workload.sla_us
        );

        const auto adaptive_decision = adaptive.update(
            workload.arrival_rate,
            workload.queue_depth,
            workload.cpu,
            workload.sla_us
        );

        std::cout << "Workload " << (i + 1) << "\n";

        std::cout << "Fixed    -> mode="
                  << mode_to_string(fixed_decision.mode)
                  << ", batch=" << fixed_decision.batch_size
                  << ", workers=" << fixed_decision.target_workers
                  << "\n";

        std::cout << "Adaptive -> mode="
                  << mode_to_string(adaptive_decision.mode)
                  << ", batch=" << adaptive_decision.batch_size
                  << ", workers=" << adaptive_decision.target_workers
                  << "\n\n";
    }

    std::cout << "Scheduler integration benchmark completed successfully.\n";

    return 0;
}
