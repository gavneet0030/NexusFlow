#pragma once

#include "core/event/event.hpp"

#include <cstddef>
#include <cstdint>

namespace nexusflow {

enum class ProcessingMode {
    SINGLE,
    MICRO_BATCH,
    PARALLEL
};

struct SchedulerMetrics {
    double arrival_rate_eps{0.0};
    std::size_t queue_depth{0};
    double cpu_utilization{0.0};
    std::uint64_t remaining_sla_us{1000000};
    std::uint64_t recent_p99_latency_us{0};
    EventPriority highest_priority{EventPriority::LOW};
};

struct SchedulingDecision {
    ProcessingMode mode{ProcessingMode::SINGLE};
    std::size_t batch_size{1};
    std::size_t target_workers{1};
    bool sla_bypass{false};
};

class Scheduler {
public:
    virtual ~Scheduler() = default;

    virtual SchedulingDecision decide(
        const SchedulerMetrics& metrics
    ) const = 0;
};

} // namespace nexusflow
