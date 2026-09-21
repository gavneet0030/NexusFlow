#include "core/processor/priority_adaptive_pipeline.hpp"

#include <chrono>
#include <cstdint>
#include <iostream>

using namespace nexusflow;

static Event make_event(uint64_t id, EventPriority priority) {
    Event event;

    event.id = id;

    event.timestamp_ns =
        static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(
                std::chrono::steady_clock::now().time_since_epoch()
            ).count()
        );

    event.priority = priority;
    event.value = static_cast<double>(id * 17);
    event.source = "priority-benchmark";
    event.type = "market-event";

    return event;
}

int main() {
    constexpr size_t event_count = 50000;

    PriorityAdaptivePipeline pipeline(4096);

    pipeline.start();

    for (size_t i = 0; i < event_count; ++i) {
        EventPriority priority = EventPriority::NORMAL;

        if (i % 1000 == 0) {
            priority = EventPriority::CRITICAL;
        } else if (i % 100 == 0) {
            priority = EventPriority::HIGH;
        } else if (i % 10 == 0) {
            priority = EventPriority::LOW;
        }

        pipeline.submit(make_event(i, priority));

        if (i % 128 == 0) {
            pipeline.process();
        }
    }

    while (pipeline.queue_depth() > 0) {
        pipeline.process();
    }

    const auto metrics = pipeline.metrics();
    const auto decision = pipeline.current_decision();

    pipeline.shutdown();

    std::cout << "\n";
    std::cout << "========================================\n";
    std::cout << "NexusFlow Priority Pipeline Benchmark\n";
    std::cout << "========================================\n";

    std::cout << "Submitted:          "
              << metrics.submitted << "\n";

    std::cout << "Accepted:           "
              << metrics.accepted << "\n";

    std::cout << "Rejected:           "
              << metrics.rejected << "\n";

    std::cout << "Processed:          "
              << metrics.processed << "\n";

    std::cout << "Critical processed: "
              << metrics.critical_processed << "\n";

    std::cout << "High processed:     "
              << metrics.high_processed << "\n";

    std::cout << "Normal processed:   "
              << metrics.normal_processed << "\n";

    std::cout << "Low processed:      "
              << metrics.low_processed << "\n";

    std::cout << "Final queue depth:  "
              << pipeline.queue_depth() << "\n";

    std::cout << "Final batch size:   "
              << decision.batch_size << "\n";

    std::cout << "Target workers:     "
              << decision.target_workers << "\n";

    std::cout << "SLA bypass:         "
              << (decision.sla_bypass ? "true" : "false") << "\n";

    std::cout << "========================================\n";

    if (metrics.processed == metrics.accepted &&
        pipeline.queue_depth() == 0) {
        std::cout << "Priority pipeline integrity: PASS\n";
        return 0;
    }

    std::cout << "Priority pipeline integrity: FAIL\n";
    return 1;
}

