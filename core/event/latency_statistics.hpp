#pragma once

#include "core/event/latency.hpp"

#include <algorithm>
#include <cstdint>
#include <vector>

namespace nexusflow {

struct LatencyPercentiles {
    double p50_us{0.0};
    double p95_us{0.0};
    double p99_us{0.0};
    double p999_us{0.0};
    double max_us{0.0};
};

inline double calculate_percentile(
    const std::vector<std::uint64_t>& input,
    double percentile
) {
    if (input.empty()) {
        return 0.0;
    }

    std::vector<std::uint64_t> values = input;

    std::sort(values.begin(), values.end());

    const double position =
        percentile * static_cast<double>(values.size() - 1);

    const std::size_t lower =
        static_cast<std::size_t>(position);

    const std::size_t upper =
        std::min(
            lower + 1,
            values.size() - 1
        );

    const double fraction =
        position - static_cast<double>(lower);

    return static_cast<double>(values[lower]) +
           fraction *
           (
               static_cast<double>(values[upper]) -
               static_cast<double>(values[lower])
           );
}

inline LatencyPercentiles calculate_latency_percentiles(
    const std::vector<std::uint64_t>& values
) {
    LatencyPercentiles result;

    if (values.empty()) {
        return result;
    }

    result.p50_us =
        calculate_percentile(values, 0.50);

    result.p95_us =
        calculate_percentile(values, 0.95);

    result.p99_us =
        calculate_percentile(values, 0.99);

    result.p999_us =
        calculate_percentile(values, 0.999);

    result.max_us =
        static_cast<double>(
            *std::max_element(
                values.begin(),
                values.end()
            )
        );

    return result;
}

}
