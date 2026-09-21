#include <chrono>
#include <cstdint>
#include <iostream>

#include "core/event/event.hpp"
#include "core/queue/priority_event_queue.hpp"

using namespace nexusflow;

static Event create_event(
    std::uint64_t id,
    EventPriority priority
) {
    Event event;

    event.id = id;

    event.timestamp_ns =
        static_cast<std::uint64_t>(
            std::chrono::duration_cast<
                std::chrono::nanoseconds
            >(
                std::chrono::steady_clock::now()
                    .time_since_epoch()
            ).count()
        );

    event.priority = priority;

    event.value =
        static_cast<double>(id);

    event.source =
        "priority-queue-benchmark";

    event.type =
        "synthetic";

    return event;
}

static const char* priority_name(
    EventPriority priority
) {
    switch (priority) {
        case EventPriority::CRITICAL:
            return "CRITICAL";

        case EventPriority::HIGH:
            return "HIGH";

        case EventPriority::NORMAL:
            return "NORMAL";

        case EventPriority::LOW:
            return "LOW";
    }

    return "UNKNOWN";
}

int main() {
    constexpr std::size_t capacity = 1024;
    constexpr std::size_t event_count = 100;

    std::cout
        << "============================================\n";

    std::cout
        << "NexusFlow Priority Queue Benchmark\n";

    std::cout
        << "============================================\n\n";

    PriorityEventQueue queue(capacity);

    // Insert lower-priority events first.
    for (std::size_t i = 0;
         i < 50;
         ++i) {

        queue.try_push(
            create_event(
                i,
                EventPriority::NORMAL
            )
        );
    }

    for (std::size_t i = 50;
         i < 75;
         ++i) {

        queue.try_push(
            create_event(
                i,
                EventPriority::HIGH
            )
        );
    }

    for (std::size_t i = 75;
         i < 95;
         ++i) {

        queue.try_push(
            create_event(
                i,
                EventPriority::LOW
            )
        );
    }

    for (std::size_t i = 95;
         i < event_count;
         ++i) {

        queue.try_push(
            create_event(
                i,
                EventPriority::CRITICAL
            )
        );
    }

    std::cout
        << "Initial queue size: "
        << queue.size()
        << "\n\n";

    bool priority_order_correct = true;

    EventPriority previous_priority =
        EventPriority::CRITICAL;

    std::size_t processed = 0;

    Event event;

    while (queue.try_pop(event)) {

        std::cout
            << "Event "
            << event.id
            << " -> "
            << priority_name(event.priority)
            << "\n";

        if (
            static_cast<int>(event.priority) >
            static_cast<int>(previous_priority)
        ) {
            priority_order_correct = false;
        }

        previous_priority =
            event.priority;

        ++processed;
    }

    std::cout << "\n";

    std::cout
        << "Processed events: "
        << processed
        << "\n";

    std::cout
        << "Final queue size: "
        << queue.size()
        << "\n";

    std::cout
        << "Priority ordering: "
        << (
            priority_order_correct
                ? "PASS"
                : "FAIL"
        )
        << "\n";

    if (
        processed == event_count &&
        priority_order_correct &&
        queue.empty()
    ) {
        std::cout
            << "Priority queue integrity: PASS\n";
    }
    else {
        std::cout
            << "Priority queue integrity: FAIL\n";
    }

    std::cout
        << "\nPriority queue benchmark completed.\n";

    return 0;
}
