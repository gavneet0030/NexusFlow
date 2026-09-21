#pragma once

#include "core/event/event.hpp"

#include <cstdint>

namespace nexusflow {

struct ProcessingResult {

    std::uint64_t event_id{0};

    std::uint64_t latency_ns{0};

    double result{0.0};
};

class EventProcessor {
public:

    ProcessingResult process(const Event& event) const;

private:

    double compute(const Event& event) const;
};

} // namespace nexusflow
