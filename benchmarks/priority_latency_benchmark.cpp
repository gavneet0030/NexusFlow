#include "core/queue/priority_event_queue.hpp"
#include "core/event/event.hpp"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <vector>

using namespace nexusflow;

struct LatencyStats {
    double p50{0.0};
    double p95{0.0};
    double p99{0.0};
    double p999{0.0};
    double max{0.0};
};

static uint64_t now_ns() {
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now().time_since_epoch()
        ).count()
    );
}

static double percentile(
    std::vector<double> values,
    double percentile_value
) {
    if (values.empty()) {
        return 0.0;
    }

    std::sort(values.begin(), values.end());

    const double index =
        (percentile_value / 100.0) *
        static_cast<double>(values.size() - 1);

    const size_t lower =
        static_cast<size_t>(index);

    const size_t upper =
        std::min(lower + 1, values.size() - 1);

    const double fraction =
        index - static_cast<double>(lower);

    return values[lower] +
           fraction * (values[upper] - values[lower]);
}

static LatencyStats calculate_stats(
    const std::vector<double>& values
) {
    LatencyStats stats;

    if (values.empty()) {
        return stats;
    }

    stats.p50 = percentile(values, 50.0);
    stats.p95 = percentile(values, 95.0);
    stats.p99 = percentile(values, 99.0);
    stats.p999 = percentile(values, 99.9);

    stats.max =
        *std::max_element(values.begin(), values.end());

    return stats;
}

static Event make_event(
    uint64_t id,
    EventPriority priority
) {
    Event event;

    event.id = id;
    event.timestamp_ns = now_ns();
    event.priority = priority;
    event.value = static_cast<double>(id);
    event.source = "latency-benchmark";
    event.type = "synthetic-event";

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
    constexpr size_t event_count = 20000;

    PriorityEventQueue queue(event_count + 100);

    std::vector<double> critical_latencies;
    std::vector<double> high_latencies;
    std::vector<double> normal_latencies;
    std::vector<double> low_latencies;
    std::vector<double> overall_latencies;

    critical_latencies.reserve(event_count);
    high_latencies.reserve(event_count);
    normal_latencies.reserve(event_count);
    low_latencies.reserve(event_count);
    overall_latencies.reserve(event_count);

    for (size_t i = 0; i < event_count; ++i) {
        EventPriority priority = EventPriority::NORMAL;

        if (i % 1000 == 0) {
            priority = EventPriority::CRITICAL;
        } else if (i % 100 == 0) {
            priority = EventPriority::HIGH;
        } else if (i % 10 == 0) {
            priority = EventPriority::LOW;
        }

        queue.try_push(make_event(i, priority));
    }

    Event event;
    size_t processed = 0;

    while (queue.try_pop(event)) {
        const uint64_t latency_ns =
            now_ns() - event.timestamp_ns;

        const double latency_us =
            static_cast<double>(latency_ns) / 1000.0;

        overall_latencies.push_back(latency_us);

        switch (event.priority) {
            case EventPriority::CRITICAL:
                critical_latencies.push_back(latency_us);
                break;

            case EventPriority::HIGH:
                high_latencies.push_back(latency_us);
                break;

            case EventPriority::NORMAL:
                normal_latencies.push_back(latency_us);
                break;

            case EventPriority::LOW:
                low_latencies.push_back(latency_us);
                break;
        }

        ++processed;
    }

    const LatencyStats critical =
        calculate_stats(critical_latencies);

    const LatencyStats high =
        calculate_stats(high_latencies);

    const LatencyStats normal =
        calculate_stats(normal_latencies);

    const LatencyStats low =
        calculate_stats(low_latencies);

    const LatencyStats overall =
        calculate_stats(overall_latencies);

    std::cout << "\n";
    std::cout << "============================================\n";
    std::cout << "NexusFlow Per-Priority Latency Benchmark\n";
    std::cout << "============================================\n";

    std::cout << "Events submitted: "
              << event_count << "\n";

    std::cout << "Events processed: "
              << processed << "\n";

    std::cout << "\n";

    std::cout << std::left
              << std::setw(12) << "Priority"
              << std::right
              << std::setw(12) << "P50(us)"
              << std::setw(12) << "P95(us)"
              << std::setw(12) << "P99(us)"
              << std::setw(12) << "P99.9(us)"
              << std::setw(12) << "Max(us)"
              << "\n";

    std::cout << "------------------------------------------------------------\n";

    auto print_stats = [](const char* name,
                          const LatencyStats& stats) {
        std::cout << std::left
                  << std::setw(12) << name
                  << std::right
                  << std::setw(12) << std::fixed
                  << std::setprecision(2) << stats.p50
                  << std::setw(12) << stats.p95
                  << std::setw(12) << stats.p99
                  << std::setw(12) << stats.p999
                  << std::setw(12) << stats.max
                  << "\n";
    };

    print_stats(
        priority_name(EventPriority::CRITICAL),
        critical
    );

    print_stats(
        priority_name(EventPriority::HIGH),
        high
    );

    print_stats(
        priority_name(EventPriority::NORMAL),
        normal
    );

    print_stats(
        priority_name(EventPriority::LOW),
        low
    );

    std::cout << "------------------------------------------------------------\n";

    print_stats("OVERALL", overall);

    std::cout << "============================================\n";

    if (processed == event_count) {
        std::cout << "Latency benchmark integrity: PASS\n";
        return 0;
    }

    std::cout << "Latency benchmark integrity: FAIL\n";
    return 1;
}
