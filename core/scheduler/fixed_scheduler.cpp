#include "core/scheduler/fixed_scheduler.hpp"

namespace nexusflow {

FixedScheduler::FixedScheduler(
    std::size_t batch_size,
    std::size_t target_workers
)
    : batch_size_(batch_size),
      target_workers_(target_workers) {
}

SchedulingDecision FixedScheduler::decide(
    const SchedulerMetrics&
) const {
    SchedulingDecision decision;

    if (batch_size_ <= 1) {
        decision.mode = ProcessingMode::SINGLE;
        decision.batch_size = 1;
    }
    else {
        decision.mode = ProcessingMode::MICRO_BATCH;
        decision.batch_size = batch_size_;
    }

    decision.target_workers = target_workers_;

    return decision;
}

} // namespace nexusflow
