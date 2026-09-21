#include "core/scheduler/scheduler_v2.hpp"

#include <iostream>
#include <iomanip>
#include <vector>
#include <string>

using namespace nexusflow;

static std::string mode_name(ProcessingMode mode)
{
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

int main()
{
    SchedulerV2Config config;

    config.scale_up_cooldown_us = 0;
    config.scale_down_cooldown_us = 0;
    config.minimum_worker_residency = 0;
    config.arrival_rate_ema_alpha = 0.25;
    config.arrival_rate_per_worker_eps = 5000.0;

    SchedulerV2 scheduler(config);

    std::cout << "========================================\n";
    std::cout << "SCHEDULER V2 ARRIVAL SCALING TEST\n";
    std::cout << "========================================\n\n";

    std::cout
        << std::left
        << std::setw(8) << "Step"
        << std::setw(14) << "InputEPS"
        << std::setw(14) << "Queue"
        << std::setw(14) << "P99us"
        << std::setw(14) << "Mode"
        << std::setw(10) << "Workers"
        << "\n";

    std::cout
        << "--------------------------------------------------------------------------\n";

    bool reached_high_capacity = false;
    bool mode_correct = true;

    for (int step = 1; step <= 20; ++step) {

        SchedulerMetrics metrics;

        metrics.arrival_rate_eps = 50000.0;
        metrics.queue_depth = 0;
        metrics.cpu_utilization = 0.0;
        metrics.remaining_sla_us = 1000000;
        metrics.recent_p99_latency_us = 20;
        metrics.highest_priority = EventPriority::LOW;

        const auto decision = scheduler.decide(metrics);

        std::cout
            << std::left
            << std::setw(8) << step
            << std::setw(14) << metrics.arrival_rate_eps
            << std::setw(14) << metrics.queue_depth
            << std::setw(14) << metrics.recent_p99_latency_us
            << std::setw(14) << mode_name(decision.mode)
            << std::setw(10) << decision.target_workers
            << "\n";

        if (decision.mode != ProcessingMode::PARALLEL) {
            mode_correct = false;
        }

        if (decision.target_workers >= 8) {
            reached_high_capacity = true;
        }
    }

    std::cout << "\n";
    std::cout << "POLICY VALIDATION\n";
    std::cout << "--------------------------------------------------------------------------\n";
    std::cout << "Sustained offered load: 50000 EPS\n";
    std::cout << "Estimated capacity: 5000 EPS per worker\n";
    std::cout << "Expected steady-state worker target: approximately 10\n";
    std::cout << "Maximum worker limit: 16\n\n";

    if (mode_correct) {
        std::cout << "PARALLEL MODE: PASS\n";
    } else {
        std::cout << "PARALLEL MODE: FAIL\n";
    }

    if (reached_high_capacity) {
        std::cout << "ARRIVAL-RATE SCALE-UP: PASS\n";
    } else {
        std::cout << "ARRIVAL-RATE SCALE-UP: FAIL\n";
    }

    const bool pass =
        mode_correct &&
        reached_high_capacity;

    std::cout << "\n";
    std::cout << "========================================\n";

    if (pass) {
        std::cout << "RESULT: PASS\n";
    } else {
        std::cout << "RESULT: FAIL\n";
    }

    std::cout << "========================================\n";

    return pass ? 0 : 1;
}
