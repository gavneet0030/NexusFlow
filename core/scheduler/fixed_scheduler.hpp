#pragma once

#include "core/scheduler/scheduler.hpp"

#include <cstddef>

namespace nexusflow {

class FixedScheduler final : public Scheduler {
public:
    FixedScheduler(
        std::size_t batch_size = 1,
        std::size_t target_workers = 1
    );

    SchedulingDecision decide(
        const SchedulerMetrics& metrics
    ) const override;

private:
    std::size_t batch_size_;
    std::size_t target_workers_;
};

} // namespace nexusflow
