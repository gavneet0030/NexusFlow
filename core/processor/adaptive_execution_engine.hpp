#pragma once

#include "core/event/event.hpp"
#include "core/processor/batch_processor.hpp"
#include "core/scheduler/scheduler.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace nexusflow {

struct ExecutionMetrics {
    std::size_t events_processed{0};

    std::uint64_t total_latency_ns{0};

    std::uint64_t max_latency_ns{0};

    double checksum{0.0};

    std::vector<std::uint64_t> latencies_ns;
};

class AdaptiveExecutionEngine {
public:
    explicit AdaptiveExecutionEngine(
        std::size_t max_workers = 16
    );

    ExecutionMetrics execute(
        const std::vector<Event>& events,
        const SchedulingDecision& decision
    );

private:
    BatchProcessor processor_;

    std::size_t max_workers_;
};

} // namespace nexusflow
