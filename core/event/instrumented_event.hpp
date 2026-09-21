#pragma once

#include "core/event/event.hpp"
#include "core/event/latency.hpp"

namespace nexusflow {

struct InstrumentedEvent {
    Event event;
    LatencyBreakdown latency;
};

}
