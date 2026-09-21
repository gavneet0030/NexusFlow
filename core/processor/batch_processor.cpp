#include "core/processor/batch_processor.hpp"

#include <algorithm>

namespace nexusflow {

BatchProcessingResult BatchProcessor::process_single(
    const Event& event
) const {
    BatchProcessingResult result;

    const ProcessingResult processed =
        processor_.process(event);

    result.events_processed = 1;
    result.total_latency_ns = processed.latency_ns;
    result.max_latency_ns = processed.latency_ns;
    result.checksum = processed.result;

    result.latencies_ns.push_back(
        processed.latency_ns
    );

    return result;
}

BatchProcessingResult BatchProcessor::process_batch(
    const std::vector<Event>& events
) const {
    BatchProcessingResult result;

    result.latencies_ns.reserve(
        events.size()
    );

    for (const Event& event : events) {
        const ProcessingResult processed =
            processor_.process(event);

        ++result.events_processed;

        result.total_latency_ns +=
            processed.latency_ns;

        result.max_latency_ns =
            std::max(
                result.max_latency_ns,
                processed.latency_ns
            );

        result.checksum +=
            processed.result;

        result.latencies_ns.push_back(
            processed.latency_ns
        );
    }

    return result;
}

} // namespace nexusflow
