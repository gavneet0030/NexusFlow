#include "core/scheduler/adaptive_scheduler.hpp"

#include <algorithm>

namespace nexusflow {

AdaptiveScheduler::AdaptiveScheduler() = default;

AdaptiveScheduler::AdaptiveScheduler(
    const SchedulerConfig& config
)
    : config_(config) {
}

AdaptiveScheduler::AdaptiveScheduler(
    std::size_t soft_queue_limit,
    std::size_t hard_queue_limit
) {
    config_.soft_queue_limit = soft_queue_limit;
    config_.hard_queue_limit = hard_queue_limit;
}

const SchedulerConfig& AdaptiveScheduler::config() const {
    return config_;
}

bool AdaptiveScheduler::is_latency_critical(
    const SchedulerMetrics& metrics
) const {
    return
        metrics.remaining_sla_us > 0 &&
        metrics.remaining_sla_us <=
            config_.latency_critical_sla_us;
}

bool AdaptiveScheduler::tail_latency_guard(
    const SchedulerMetrics& metrics
) const {
    if (
        metrics.remaining_sla_us <=
        config_.tail_guard_sla_us
    ) {
        return true;
    }

    if (
        metrics.remaining_sla_us <=
            config_.latency_critical_sla_us &&
        metrics.queue_depth >=
            config_.tail_guard_queue_threshold
    ) {
        return true;
    }

    return false;
}

SchedulingDecision AdaptiveScheduler::decide(
    const SchedulerMetrics& metrics
) const {
    SchedulingDecision decision;

    decision.target_workers =
        calculate_worker_target(metrics);

    if (
        metrics.highest_priority ==
        EventPriority::CRITICAL
    ) {
        decision.mode =
            ProcessingMode::SINGLE;

        decision.batch_size = 1;

        decision.target_workers =
            std::max(
                decision.target_workers,
                config_.critical_min_workers
            );

        decision.sla_bypass = true;

        return decision;
    }

    if (
        metrics.highest_priority ==
        EventPriority::HIGH
    ) {
        decision.mode =
            ProcessingMode::MICRO_BATCH;

        decision.batch_size =
            config_.high_priority_batch_size;

        decision.target_workers =
            std::max(
                decision.target_workers,
                config_.high_priority_min_workers
            );

        return decision;
    }

    if (is_latency_critical(metrics)) {
        decision.mode =
            ProcessingMode::SINGLE;

        decision.batch_size = 1;

        decision.sla_bypass = true;

        decision.target_workers =
            std::max(
                decision.target_workers,
                config_.latency_critical_min_workers
            );

        return decision;
    }

    if (tail_latency_guard(metrics)) {
        decision.mode =
            ProcessingMode::SINGLE;

        decision.batch_size = 1;

        decision.sla_bypass = true;

        decision.target_workers =
            std::max(
                decision.target_workers,
                config_.latency_critical_min_workers
            );

        return decision;
    }

    if (
        metrics.queue_depth <
            config_.low_queue_threshold &&
        metrics.arrival_rate_eps <
            config_.low_arrival_rate_eps
    ) {
        decision.mode =
            ProcessingMode::SINGLE;

        decision.batch_size = 1;

        return decision;
    }

    if (
        metrics.queue_depth <
            config_.soft_queue_limit &&
        metrics.arrival_rate_eps <
            config_.moderate_arrival_rate_eps
    ) {
        decision.mode =
            ProcessingMode::MICRO_BATCH;

        decision.batch_size =
            config_.micro_batch_size;

        return decision;
    }

    decision.mode =
        ProcessingMode::PARALLEL;

    decision.batch_size =
        config_.parallel_batch_size;

    return decision;
}

std::size_t
AdaptiveScheduler::calculate_worker_target(
    const SchedulerMetrics& metrics
) const {
    if (
        metrics.highest_priority ==
        EventPriority::CRITICAL
    ) {
        return 8;
    }

    if (is_latency_critical(metrics)) {
        if (
            metrics.queue_depth >=
            config_.hard_queue_limit
        ) {
            return 16;
        }

        if (
            metrics.queue_depth >=
            config_.soft_queue_limit
        ) {
            return 8;
        }

        return 2;
    }

    if (
        metrics.queue_depth >=
        config_.hard_queue_limit
    ) {
        return 16;
    }

    if (
        metrics.queue_depth >=
        config_.extreme_worker_queue_threshold
    ) {
        return 8;
    }

    if (
        metrics.queue_depth >=
        config_.high_worker_queue_threshold
    ) {
        return 4;
    }

    if (
        metrics.queue_depth >=
        config_.worker_queue_threshold
    ) {
        return 2;
    }

    return 1;
}

} // namespace nexusflow
