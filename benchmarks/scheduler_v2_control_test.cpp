#include "core/scheduler/scheduler_v2.hpp"

#include <iostream>
#include <string>

using namespace nexusflow;

static SchedulingDecision run(
    SchedulerV2& scheduler,
    SchedulerMetrics metrics,
    const std::string& label)
{
    const auto decision =
        scheduler.decide(metrics);

    std::cout
        << label
        << " | queue=" << metrics.queue_depth
        << " | arrival=" << metrics.arrival_rate_eps
        << " | p99=" << metrics.recent_p99_latency_us
        << " | workers=" << decision.target_workers
        << " | batch=" << decision.batch_size
        << " | sla_bypass="
        << (decision.sla_bypass ? "true" : "false")
        << '\n';

    return decision;
}

int main()
{
    SchedulerV2Config config;

    config.maximum_workers = 16;

    config.scale_up_queue_growth_threshold = 2.0;
    config.scale_down_queue_growth_threshold = -1.0;

    config.scale_up_cooldown_us = 0;
    config.scale_down_cooldown_us = 0;

    config.minimum_worker_residency = 1;

    config.low_load_scale_down_threshold = 2000.0;
    config.low_queue_scale_down_threshold = 16;
    config.low_load_scale_down_latency_us = 500;

    SchedulerV2 scheduler(config);

    SchedulerMetrics metrics;

    std::cout << "========================================\n";
    std::cout << "SCHEDULER V2 CONTROL VALIDATION\n";
    std::cout << "========================================\n\n";

    metrics.arrival_rate_eps = 500.0;
    metrics.queue_depth = 2;
    metrics.remaining_sla_us = 5000;
    metrics.recent_p99_latency_us = 20;
    metrics.highest_priority = EventPriority::LOW;

    auto low =
        run(scheduler, metrics, "LOW");

    if (low.target_workers != 1) {
        std::cerr << "FAIL: Low load did not start with one worker.\n";
        return 1;
    }

    metrics.arrival_rate_eps = 15000.0;
    metrics.queue_depth = 100;
    metrics.recent_p99_latency_us = 100;

    auto grow1 =
        run(scheduler, metrics, "GROW-1");

    metrics.queue_depth = 300;

    auto grow2 =
        run(scheduler, metrics, "GROW-2");

    if (grow2.target_workers <= grow1.target_workers) {
        std::cerr << "FAIL: Queue growth did not increase workers.\n";
        return 2;
    }

    metrics.arrival_rate_eps = 8000.0;
    metrics.queue_depth = 40;
    metrics.recent_p99_latency_us = 5000;

    auto tail =
        run(scheduler, metrics, "TAIL");

    if (tail.target_workers < grow2.target_workers) {
        std::cerr << "FAIL: Tail latency did not protect worker capacity.\n";
        return 3;
    }

    metrics.arrival_rate_eps = 100.0;
    metrics.queue_depth = 0;
    metrics.recent_p99_latency_us = 20;
    metrics.remaining_sla_us = 5000;

    std::size_t previous_workers =
        tail.target_workers;

    bool scaled_down = false;

    for (int i = 1; i <= 24; ++i) {

        auto recovery =
            run(
                scheduler,
                metrics,
                "RECOVERY-" +
                    std::to_string(i));

        if (recovery.target_workers <
            previous_workers) {

            scaled_down = true;
        }

        previous_workers =
            recovery.target_workers;
    }

    if (!scaled_down) {
        std::cerr
            << "FAIL: Scheduler did not scale down during recovery.\n";
        return 4;
    }

    metrics.arrival_rate_eps = 50000.0;
    metrics.queue_depth = 0;
    metrics.recent_p99_latency_us = 20;

    auto high_rate_empty =
        run(
            scheduler,
            metrics,
            "HIGH-RATE-EMPTY");

    if (high_rate_empty.target_workers >
        previous_workers + 2) {

        std::cerr
            << "FAIL: High arrival rate alone caused uncontrolled scaling.\n";
        return 5;
    }

    metrics.arrival_rate_eps = 1000.0;
    metrics.queue_depth = 0;
    metrics.recent_p99_latency_us = 50;
    metrics.remaining_sla_us = 300;

    auto sla =
        run(
            scheduler,
            metrics,
            "SLA-CRITICAL");

    if (!sla.sla_bypass) {
        std::cerr
            << "FAIL: SLA bypass was not activated.\n";
        return 6;
    }

    std::cout << "\n========================================\n";
    std::cout << "ALL CONTROL TESTS PASSED\n";
    std::cout << "========================================\n";

    return 0;
}