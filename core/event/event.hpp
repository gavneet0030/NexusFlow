#pragma once

#include <cstdint>
#include <string>

namespace nexusflow {

enum class EventPriority : std::uint8_t {
    LOW = 0,
    NORMAL = 1,
    HIGH = 2,
    CRITICAL = 3
};

struct Event {

    std::uint64_t id{0};

    std::uint64_t timestamp_ns{0};

    EventPriority priority{
        EventPriority::NORMAL
    };

    double value{0.0};

    std::string source;

    std::string type;

    // End-to-end latency instrumentation.
    // Mutable so the existing const Event& processing interface
    // does not need to change.
    mutable std::uint64_t created_at_ns{0};
    mutable std::uint64_t accepted_at_ns{0};
    mutable std::uint64_t dequeued_at_ns{0};
    mutable std::uint64_t processing_start_ns{0};
    mutable std::uint64_t processing_end_ns{0};
    mutable std::uint64_t completed_at_ns{0};
};

} // namespace nexusflow
