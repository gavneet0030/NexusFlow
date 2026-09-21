#include "core/scheduler/scheduler_controller.hpp"

#include <algorithm>
#include <utility>

namespace nexusflow {

SchedulerController::SchedulerController(
    std::unique_ptr<Scheduler> scheduler,
    std::size_t max_workers
)
    : scheduler_(std::move(scheduler)),
      max_workers_(std::max<std::size_t>(1, max_workers)) {
    last_decision_.mode = ProcessingMode::SINGLE;
    last_decision_.batch_size = 1;
    last_decision_.target_workers = 1;
}

SchedulingDecision SchedulerController::update(
    double arrival_rate_eps,
    std::size_t queue_depth,
    double cpu_utilization,
    std::uint64_t remaining_sla_us
) {
    if (!scheduler_) {
        return last_decision_;
    }

    SchedulerMetrics metrics;

    metrics.arrival_rate_eps = arrival_rate_eps;
    metrics.queue_depth = queue_depth;
    metrics.cpu_utilization = cpu_utilization;
    metrics.remaining_sla_us = remaining_sla_us;

    last_decision_ = scheduler_->decide(metrics);

    last_decision_.target_workers =
        std::clamp(
            last_decision_.target_workers,
            std::size_t{1},
            max_workers_
        );

    return last_decision_;
}

const SchedulingDecision& SchedulerController::last_decision() const {
    return last_decision_;
}

} // namespace nexusflow
