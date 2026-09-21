#pragma once

#include "core/event/event.hpp"
#include "core/processor/event_processor.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace nexusflow {

struct BatchProcessingResult {
    std::size_t events_processed{0};
    std::uint64_t total_latency_ns{0};
    std::uint64_t max_latency_ns{0};
    double checksum{0.0};

    std::vector<std::uint64_t> latencies_ns;
};

class BatchProcessor {
public:
    BatchProcessingResult process_single(
        const Event& event
    ) const;

    BatchProcessingResult process_batch(
        const std::vector<Event>& events
    ) const;

private:
    EventProcessor processor_;
};

} // namespace nexusflow
