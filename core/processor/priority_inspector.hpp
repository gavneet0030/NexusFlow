#pragma once

#include <cstddef>
#include <vector>

#include "core/event/event.hpp"

namespace nexusflow {

class PriorityInspector {
public:
    static EventPriority highest_priority(
        const std::vector<Event>& events
    );
};

} // namespace nexusflow
