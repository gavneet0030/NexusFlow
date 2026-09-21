#include "core/processor/priority_inspector.hpp"

namespace nexusflow {

EventPriority PriorityInspector::highest_priority(
    const std::vector<Event>& events
) {
    EventPriority highest =
        EventPriority::LOW;

    for (const Event& event : events) {
        if (
            static_cast<int>(event.priority) >
            static_cast<int>(highest)
        ) {
            highest = event.priority;
        }
    }

    return highest;
}

} // namespace nexusflow
