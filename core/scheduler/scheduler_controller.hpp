#pragma once

#include "core/scheduler/scheduler.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>

namespace nexusflow {

class SchedulerController {
public:
    SchedulerController(
        std::unique_ptr<Scheduler> scheduler,
        std::size_t max_workers
    );

    SchedulingDecision update(
        double arrival_rate_eps,
        std::size_t queue_depth,
        double cpu_utilization,
        std::uint64_t remaining_sla_us
    );

    const SchedulingDecision& last_decision() const;

private:
    std::unique_ptr<Scheduler> scheduler_;
    std::size_t max_workers_;
    SchedulingDecision last_decision_{};
};

} // namespace nexusflow
