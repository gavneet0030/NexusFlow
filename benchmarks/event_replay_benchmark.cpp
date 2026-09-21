#include "streaming/event_replay/event_replay.hpp"
using nexusflow::streaming::EventReplay;

#include <cstdint>
#include <iomanip>
#include <iostream>
#include <string>

namespace {

std::string priority_to_string(
    nexusflow::EventPriority priority
) {
    switch (priority) {
        case nexusflow::EventPriority::LOW:
            return "LOW";

        case nexusflow::EventPriority::NORMAL:
            return "NORMAL";

        case nexusflow::EventPriority::HIGH:
            return "HIGH";

        case nexusflow::EventPriority::CRITICAL:
            return "CRITICAL";
    }

    return "UNKNOWN";
}

}

int main() {
    using namespace nexusflow;

    const std::string dataset_path =
        "data/raw/event_replay_sample.csv";

    EventReplay replay;

    std::cout
        << "=============================================\n"
        << "NexusFlow Event Replay Validation\n"
        << "=============================================\n\n";

    if (!replay.load_csv(dataset_path)) {
        std::cerr
            << "Failed to load replay dataset: "
            << dataset_path
            << "\n";

        return 1;
    }

    std::cout
        << "Dataset loaded successfully.\n"
        << "Records loaded: "
        << replay.size()
        << "\n\n";

    std::uint64_t previous_timestamp_ns = 0;

    for (
        std::size_t index = 0;
        index < replay.size();
        ++index
    ) {
        const Event event =
            replay.make_event(index);

        std::uint64_t delta_ns = 0;

        if (index > 0) {
            delta_ns =
                event.timestamp_ns -
                previous_timestamp_ns;
        }

        std::cout
            << "Event "
            << std::setw(2)
            << event.id
            << " | timestamp_ns="
            << event.timestamp_ns
            << " | delta_ns="
            << delta_ns
            << " | priority="
            << priority_to_string(event.priority)
            << " | value="
            << std::fixed
            << std::setprecision(2)
            << event.value
            << " | source="
            << event.source
            << " | type="
            << event.type
            << "\n";

        previous_timestamp_ns =
            event.timestamp_ns;
    }

    std::cout
        << "\n=============================================\n"
        << "Replay validation: PASS\n"
        << "Events converted: "
        << replay.size()
        << "\n"
        << "=============================================\n";

    return 0;
}
