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

    SchedulerV2 scheduler(config);

    struct Scenario {
        double arrival_rate;
        std::size_t queue_depth;
        std::uint64_t p99_latency_us;
    };

    const std::vector<Scenario> scenarios = {
        {1000.0, 0, 100},
        {5000.0, 0, 100},
        {10000.0, 0, 100},
        {20000.0, 0, 100},
        {30000.0, 0, 100},
        {40000.0, 0, 100},
        {50000.0, 0, 100}
    };

    std::cout
        << std::left
        << std::setw(12) << "ArrivalEPS"
        << std::setw(12) << "Queue"
        << std::setw(12) << "P99us"
        << std::setw(14) << "Mode"
        << std::setw(12) << "Workers"
        << "\n";

    std::cout << "------------------------------------------------------------\n";

    bool pass = true;

    for (const auto& scenario : scenarios) {
        SchedulerMetrics metrics;

        metrics.arrival_rate_eps = scenario.arrival_rate;
        metrics.queue_depth = scenario.queue_depth;
        metrics.cpu_utilization = 0.0;
        metrics.remaining_sla_us = 1000000;
        metrics.recent_p99_latency_us = scenario.p99_latency_us;
        metrics.highest_priority = EventPriority::LOW;

        const auto decision = scheduler.decide(metrics);

        std::cout
            << std::left
            << std::setw(12) << scenario.arrival_rate
            << std::setw(12) << scenario.queue_depth
            << std::setw(12) << scenario.p99_latency_us
            << std::setw(14) << mode_name(decision.mode)
            << std::setw(12) << decision.target_workers
            << "\n";

        if (scenario.arrival_rate >= 5000.0 &&
            decision.mode != ProcessingMode::PARALLEL) {
            pass = false;
        }
    }

    std::cout << "\n";
    std::cout << "POLICY CHECK\n";
    std::cout << "------------------------------------------------------------\n";
    std::cout << "Arrival rate >= 5000 EPS must select PARALLEL mode.\n";
    std::cout << "Queue depth remains zero.\n";
    std::cout << "Processing P99 remains low.\n";
    std::cout << "This isolates arrival-rate scaling from queue-pressure scaling.\n";
    std::cout << "\n";

    if (pass) {
        std::cout << "RESULT: PASS\n";
    } else {
        std::cout << "RESULT: FAIL\n";
    }

    std::cout << "\n";
    std::cout << "EXPECTED FINDING:\n";
    std::cout << "High arrival rate changes the processing mode but does not\n";
    std::cout << "directly increase the worker target beyond the minimum.\n";

    return pass ? 0 : 1;
}
