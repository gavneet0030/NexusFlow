#pragma once

#include "core/scheduler/scheduler.hpp"

#include <cstddef>
#include <cstdint>

namespace nexusflow {

struct SchedulerConfig {
    std::size_t soft_queue_limit{256};
    std::size_t hard_queue_limit{2048};

    std::size_t low_queue_threshold{8};
    std::size_t worker_queue_threshold{32};
    std::size_t high_worker_queue_threshold{128};
    std::size_t extreme_worker_queue_threshold{512};

    double low_arrival_rate_eps{1000.0};
    double moderate_arrival_rate_eps{5000.0};

    std::uint64_t latency_critical_sla_us{1000};
    std::uint64_t tail_guard_sla_us{250};
    std::size_t tail_guard_queue_threshold{32};

    std::size_t high_priority_batch_size{4};
    std::size_t micro_batch_size{32};
    std::size_t parallel_batch_size{64};

    std::size_t critical_min_workers{4};
    std::size_t high_priority_min_workers{2};
    std::size_t latency_critical_min_workers{2};
};

class AdaptiveScheduler final : public Scheduler {
public:
    AdaptiveScheduler();

    explicit AdaptiveScheduler(
        const SchedulerConfig& config
    );

    AdaptiveScheduler(
        std::size_t soft_queue_limit,
        std::size_t hard_queue_limit
    );

    SchedulingDecision decide(
        const SchedulerMetrics& metrics
    ) const override;

    const SchedulerConfig& config() const;

private:
    std::size_t calculate_worker_target(
        const SchedulerMetrics& metrics
    ) const;

    bool is_latency_critical(
        const SchedulerMetrics& metrics
    ) const;

    bool tail_latency_guard(
        const SchedulerMetrics& metrics
    ) const;

    SchedulerConfig config_;
};

} // namespace nexusflow
