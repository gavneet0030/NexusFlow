#include "core/processor/event_processor.hpp"
#include <cstdint>

#include <chrono>

namespace nexusflow {

ProcessingResult EventProcessor::process(const Event& event) const {

    const double computation_result = compute(event);

    const auto now = std::chrono::steady_clock::now();    
    const std::uint64_t now_ns =
        static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(
                now.time_since_epoch()
            ).count()
        );

    ProcessingResult result;

    result.event_id = event.id;
    result.result = computation_result;

    if (now_ns >= event.timestamp_ns) {
        result.latency_ns = now_ns - event.timestamp_ns;
    }

    return result;
}


/*
 * Synthetic CPU workload.
 *
 * This represents work that a real event-processing engine might
 * perform, such as:
 *
 * - validation
 * - feature calculation
 * - risk calculation
 * - rule evaluation
 *
 * The workload is intentionally deterministic so that benchmarks
 * remain reproducible.
 */
double EventProcessor::compute(const Event& event) const {

    double value = event.value;

    for (int i = 0; i < 32; ++i) {

        value =
            (value * 1.000001) +
            static_cast<double>((event.id + i) % 17) * 0.00001;

        value =
            value -
            static_cast<double>(static_cast<std::uint8_t>(event.priority))
            * 0.000001;
    }

    return value;
}

} // namespace nexusflow
