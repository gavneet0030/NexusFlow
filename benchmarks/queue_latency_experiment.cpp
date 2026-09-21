#include <algorithm>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <numeric>
#include <queue>
#include <string>
#include <vector>

#include "core/event/event.hpp"
#include "core/queue/priority_event_queue.hpp"

using namespace nexusflow;

struct LatencySample {
    EventPriority priority;
    std::uint64_t latency_ns;
};

static std::uint64_t now_ns() {
    return static_cast<std::uint64_t>(
        std::chrono::duration_cast<
            std::chrono::nanoseconds
        >(
            std::chrono::steady_clock::now().time_since_epoch()
        ).count()
    );
}

static Event create_event(
    std::uint64_t id,
    EventPriority priority
) {
    Event event;

    event.id = id;
    event.timestamp_ns = now_ns();
    event.priority = priority;
    event.value = static_cast<double>(id);
    event.source = "queue-latency-experiment";
    event.type = "synthetic";

    return event;
}

static const char* priority_name(
    EventPriority priority
) {
    switch (priority) {
        case EventPriority::LOW:
            return "LOW";

        case EventPriority::NORMAL:
            return "NORMAL";

        case EventPriority::HIGH:
            return "HIGH";

        case EventPriority::CRITICAL:
            return "CRITICAL";
    }

    return "UNKNOWN";
}

static double percentile(
    std::vector<std::uint64_t> values,
    double p
) {
    if (values.empty()) {
        return 0.0;
    }

    std::sort(
        values.begin(),
        values.end()
    );

    const double position =
        p * static_cast<double>(
            values.size() - 1
        );

    const std::size_t lower =
        static_cast<std::size_t>(position);

    const std::size_t upper =
        std::min(
            lower + 1,
            values.size() - 1
        );

    const double fraction =
        position - static_cast<double>(lower);

    return static_cast<double>(
        values[lower]
    ) * (1.0 - fraction)
    +
    static_cast<double>(
        values[upper]
    ) * fraction;
}

static void print_statistics(
    const std::string& name,
    const std::vector<LatencySample>& samples
) {
    if (samples.empty()) {
        std::cout
            << name
            << ": no samples\n";

        return;
    }

    std::vector<std::uint64_t> latencies;

    latencies.reserve(samples.size());

    for (const auto& sample : samples) {
        latencies.push_back(
            sample.latency_ns
        );
    }

    const std::uint64_t total =
        std::accumulate(
            latencies.begin(),
            latencies.end(),
            std::uint64_t{0}
        );

    const double average =
        static_cast<double>(total) /
        static_cast<double>(latencies.size());

    std::cout
        << name
        << "\n";

    std::cout
        << "Samples: "
        << latencies.size()
        << "\n";

    std::cout
        << "Average latency: "
        << average / 1000.0
        << " us\n";

    std::cout
        << "P50 latency: "
        << percentile(latencies, 0.50) / 1000.0
        << " us\n";

    std::cout
        << "P95 latency: "
        << percentile(latencies, 0.95) / 1000.0
        << " us\n";

    std::cout
        << "P99 latency: "
        << percentile(latencies, 0.99) / 1000.0
        << " us\n";

    std::cout
        << "P99.9 latency: "
        << percentile(latencies, 0.999) / 1000.0
        << " us\n";

    std::cout
        << "Maximum latency: "
        << static_cast<double>(
            *std::max_element(
                latencies.begin(),
                latencies.end()
            )
        ) / 1000.0
        << " us\n";

    std::cout
        << "--------------------------------------------\n";
}

static std::vector<Event> create_workload() {
    constexpr std::size_t event_count = 10000;

    std::vector<Event> events;

    events.reserve(event_count);

    for (std::size_t i = 0;
         i < event_count;
         ++i) {

        EventPriority priority =
            EventPriority::NORMAL;

        if (i % 1000 == 0) {
            priority =
                EventPriority::CRITICAL;
        }
        else if (i % 100 == 0) {
            priority =
                EventPriority::HIGH;
        }
        else if (i % 10 == 0) {
            priority =
                EventPriority::LOW;
        }

        events.push_back(
            create_event(
                static_cast<std::uint64_t>(i),
                priority
            )
        );
    }

    return events;
}

static void simulate_processing(
    const Event& event
) {
    volatile double value =
        event.value;

    for (int i = 0; i < 200; ++i) {
        value =
            value * 1.000001 +
            static_cast<double>(i % 7);
    }

    (void)value;
}

static std::vector<LatencySample> run_fifo(
    const std::vector<Event>& workload
) {
    std::queue<Event> queue;

    for (const auto& event : workload) {
        queue.push(event);
    }

    std::vector<LatencySample> samples;

    samples.reserve(workload.size());

    while (!queue.empty()) {
        Event event =
            std::move(queue.front());

        queue.pop();

        const std::uint64_t latency =
            now_ns() - event.timestamp_ns;

        samples.push_back({
            event.priority,
            latency
        });

        simulate_processing(event);
    }

    return samples;
}

static std::vector<LatencySample> run_priority(
    const std::vector<Event>& workload
) {
    PriorityEventQueue queue(
        workload.size() + 1
    );

    for (const auto& event : workload) {
        queue.try_push(event);
    }

    std::vector<LatencySample> samples;

    samples.reserve(workload.size());

    Event event;

    while (queue.try_pop(event)) {
        const std::uint64_t latency =
            now_ns() - event.timestamp_ns;

        samples.push_back({
            event.priority,
            latency
        });

        simulate_processing(event);
    }

    return samples;
}

static std::vector<LatencySample> filter_priority(
    const std::vector<LatencySample>& samples,
    EventPriority priority
) {
    std::vector<LatencySample> result;

    for (const auto& sample : samples) {
        if (sample.priority == priority) {
            result.push_back(sample);
        }
    }

    return result;
}

static double average_latency(
    const std::vector<LatencySample>& samples
) {
    if (samples.empty()) {
        return 0.0;
    }

    std::uint64_t total = 0;

    for (const auto& sample : samples) {
        total += sample.latency_ns;
    }

    return static_cast<double>(total) /
           static_cast<double>(samples.size());
}

int main() {
    std::cout
        << "============================================\n";

    std::cout
        << "NexusFlow Queue Latency Experiment\n";

    std::cout
        << "============================================\n\n";

    const auto workload =
        create_workload();

    const std::size_t critical_count =
        static_cast<std::size_t>(
            std::count_if(
                workload.begin(),
                workload.end(),
                [](const Event& event) {
                    return event.priority ==
                        EventPriority::CRITICAL;
                }
            )
        );

    std::cout
        << "Workload events: "
        << workload.size()
        << "\n";

    std::cout
        << "Critical events: "
        << critical_count
        << "\n\n";

    std::cout
        << "Running FIFO experiment...\n\n";

    const auto fifo_samples =
        run_fifo(workload);

    print_statistics(
        "FIFO - All Events",
        fifo_samples
    );

    const auto fifo_critical =
        filter_priority(
            fifo_samples,
            EventPriority::CRITICAL
        );

    print_statistics(
        "FIFO - Critical Events",
        fifo_critical
    );

    std::cout
        << "\nRunning priority queue experiment...\n\n";

    const auto priority_samples =
        run_priority(workload);

    print_statistics(
        "Priority Queue - All Events",
        priority_samples
    );

    const auto priority_critical =
        filter_priority(
            priority_samples,
            EventPriority::CRITICAL
        );

    print_statistics(
        "Priority Queue - Critical Events",
        priority_critical
    );

    const double fifo_average =
        average_latency(
            fifo_critical
        );

    const double priority_average =
        average_latency(
            priority_critical
        );

    double improvement = 0.0;

    if (fifo_average > 0.0) {
        improvement =
            (
                fifo_average -
                priority_average
            )
            /
            fifo_average
            *
            100.0;
    }

    std::cout
        << "\n============================================\n";

    std::cout
        << "Critical Event Comparison\n";

    std::cout
        << "============================================\n";

    std::cout
        << "FIFO average: "
        << fifo_average / 1000.0
        << " us\n";

    std::cout
        << "Priority average: "
        << priority_average / 1000.0
        << " us\n";

    std::cout
        << "Latency improvement: "
        << improvement
        << "%\n";

    if (priority_average < fifo_average) {
        std::cout
            << "Priority benefit: CONFIRMED\n";
    }
    else {
        std::cout
            << "Priority benefit: NOT CONFIRMED\n";
    }

    std::ofstream output(
        "benchmarks/results/queue_latency_comparison.csv"
    );

    if (output.is_open()) {
        output
            << "queue_type,event_priority,latency_ns\n";

        for (const auto& sample : fifo_samples) {
            output
                << "FIFO,"
                << priority_name(sample.priority)
                << ","
                << sample.latency_ns
                << "\n";
        }

        for (const auto& sample : priority_samples) {
            output
                << "PRIORITY,"
                << priority_name(sample.priority)
                << ","
                << sample.latency_ns
                << "\n";
        }

        output.close();

        std::cout
            << "\nResults saved to:\n"
            << "benchmarks/results/queue_latency_comparison.csv\n";
    }

    std::cout
        << "\nQueue latency experiment completed.\n";

    return 0;
}
