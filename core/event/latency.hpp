#pragma once

#include <chrono>
#include <cstdint>

namespace nexusflow {

inline std::uint64_t steady_clock_ns() {
    return static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now().time_since_epoch()
        ).count()
    );
}

struct LatencyBreakdown {
    std::uint64_t event_created_ns{0};
    std::uint64_t accepted_ns{0};
    std::uint64_t processing_start_ns{0};
    std::uint64_t processing_end_ns{0};
    std::uint64_t completed_ns{0};

    std::uint64_t queue_wait_us() const {
        if (processing_start_ns < accepted_ns) {
            return 0;
        }

        return (processing_start_ns - accepted_ns) / 1000;
    }

    std::uint64_t processing_us() const {
        if (processing_end_ns < processing_start_ns) {
            return 0;
        }

        return (processing_end_ns - processing_start_ns) / 1000;
    }

    std::uint64_t completion_us() const {
        if (completed_ns < processing_end_ns) {
            return 0;
        }

        return (completed_ns - processing_end_ns) / 1000;
    }

    std::uint64_t end_to_end_us() const {
        if (completed_ns < event_created_ns) {
            return 0;
        }

        return (completed_ns - event_created_ns) / 1000;
    }
};

}
