#include "core/event/event.hpp"
#include "core/queue/bounded_mpmc_queue.hpp"
#include "core/queue/backpressure_policy.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <numeric>
#include <string>
#include <thread>
#include <vector>

using namespace nexusflow;

struct LatencyStats {
    double p50_us{0.0};
    double p95_us{0.0};
    double p99_us{0.0};
    double p999_us{0.0};
    double max_us{0.0};
};

double percentile(std::vector<uint64_t> values, double percentile_value)
{
    if (values.empty()) {
        return 0.0;
    }

    std::sort(values.begin(), values.end());

    const double rank =
        (percentile_value / 100.0) * static_cast<double>(values.size() - 1);

    const size_t lower = static_cast<size_t>(rank);
    const size_t upper = std::min(lower + 1, values.size() - 1);

    const double fraction = rank - static_cast<double>(lower);

    return static_cast<double>(values[lower]) +
           fraction * static_cast<double>(values[upper] - values[lower]);
}

LatencyStats calculate_latency_stats(const std::vector<uint64_t>& latencies)
{
    LatencyStats stats;

    stats.p50_us = percentile(latencies, 50.0);
    stats.p95_us = percentile(latencies, 95.0);
    stats.p99_us = percentile(latencies, 99.0);
    stats.p999_us = percentile(latencies, 99.9);

    if (!latencies.empty()) {
        stats.max_us =
            static_cast<double>(*std::max_element(latencies.begin(), latencies.end()));
    }

    return stats;
}

struct Scenario {
    std::string name;
    size_t producers;
    size_t events_per_producer;
    size_t queue_capacity;
};

struct ScenarioResult {
    std::string name;
    uint64_t submitted{0};
    uint64_t accepted{0};
    uint64_t rejected{0};
    uint64_t processed{0};
    double throughput_eps{0.0};
    LatencyStats latency;
};

ScenarioResult run_scenario(const Scenario& scenario)
{
    BoundedMPMCQueue<Event> queue(scenario.queue_capacity);
    BackpressurePolicy backpressure(
        static_cast<size_t>(scenario.queue_capacity * 0.70),
        static_cast<size_t>(scenario.queue_capacity * 0.90)
    );

    const uint64_t total_events =
        static_cast<uint64_t>(scenario.producers) *
        static_cast<uint64_t>(scenario.events_per_producer);

    std::atomic<uint64_t> submitted{0};
    std::atomic<uint64_t> accepted{0};
    std::atomic<uint64_t> rejected{0};
    std::atomic<uint64_t> processed{0};

    std::atomic<bool> producers_finished{false};

    std::vector<uint64_t> latencies;
    std::mutex latency_mutex;

    std::thread consumer([&]() {
        Event event;

        while (true) {
            if (queue.try_pop(event)) {
                const auto now =
                    std::chrono::steady_clock::now();

                const uint64_t now_ns =
                    static_cast<uint64_t>(
                        std::chrono::duration_cast<std::chrono::nanoseconds>(
                            now.time_since_epoch()
                        ).count()
                    );

                uint64_t latency_us = 0;

                if (now_ns >= event.timestamp_ns) {
                    latency_us = (now_ns - event.timestamp_ns) / 1000ULL;
                }

                {
                    std::lock_guard<std::mutex> lock(latency_mutex);
                    latencies.push_back(latency_us);
                }

                volatile uint64_t work = 0;

                for (uint64_t i = 0; i < 100; ++i) {
                    work += (event.id + i) % 97;
                }

                (void)work;

                processed.fetch_add(1, std::memory_order_relaxed);
            }
            else {
                if (producers_finished.load(std::memory_order_acquire) &&
                    queue.empty()) {
                    break;
                }

                std::this_thread::yield();
            }
        }
    });

    std::vector<std::thread> producers;
    producers.reserve(scenario.producers);

    const auto start = std::chrono::steady_clock::now();

    for (size_t producer_id = 0;
         producer_id < scenario.producers;
         ++producer_id) {

        producers.emplace_back([&, producer_id]() {
            for (size_t i = 0;
                 i < scenario.events_per_producer;
                 ++i) {

                const uint64_t event_id =
                    static_cast<uint64_t>(producer_id) *
                    static_cast<uint64_t>(scenario.events_per_producer) +
                    static_cast<uint64_t>(i);

                Event event;
                event.id = event_id;

                const auto now =
                    std::chrono::steady_clock::now();

                event.timestamp_ns =
                    static_cast<uint64_t>(
                        std::chrono::duration_cast<std::chrono::nanoseconds>(
                            now.time_since_epoch()
                        ).count()
                    );

                event.priority = EventPriority::NORMAL;
                event.value = static_cast<double>(event_id);
                event.source = "latency-load-test";
                event.type = "synthetic";

                submitted.fetch_add(1, std::memory_order_relaxed);

                const auto decision =
                    backpressure.evaluate(
                        queue.size(),
                        queue.capacity()
                    );

                if (decision.action == BackpressureAction::REJECT) {
                    rejected.fetch_add(1, std::memory_order_relaxed);
                    continue;
                }

                if (decision.action == BackpressureAction::THROTTLE) {
                    std::this_thread::yield();
                }

                if (queue.try_push(std::move(event))) {
                    accepted.fetch_add(1, std::memory_order_relaxed);
                }
                else {
                    rejected.fetch_add(1, std::memory_order_relaxed);
                }
            }
        });
    }

    for (auto& producer : producers) {
        producer.join();
    }

    producers_finished.store(true, std::memory_order_release);

    consumer.join();

    const auto end = std::chrono::steady_clock::now();

    const double elapsed_seconds =
        std::chrono::duration<double>(end - start).count();

    ScenarioResult result;
    result.name = scenario.name;
    result.submitted = submitted.load();
    result.accepted = accepted.load();
    result.rejected = rejected.load();
    result.processed = processed.load();

    if (elapsed_seconds > 0.0) {
        result.throughput_eps =
            static_cast<double>(result.processed) /
            elapsed_seconds;
    }

    result.latency = calculate_latency_stats(latencies);

    return result;
}

int main()
{
    std::vector<Scenario> scenarios = {
        {"LOW_LOAD", 1, 10000, 256},
        {"MEDIUM_LOAD", 2, 25000, 256},
        {"HIGH_LOAD", 4, 50000, 256},
        {"OVERLOAD", 8, 50000, 256}
    };

    std::vector<ScenarioResult> results;

    std::cout << "=============================================\n";
    std::cout << "NexusFlow Latency Under Load Benchmark\n";
    std::cout << "=============================================\n\n";

    for (const auto& scenario : scenarios) {
        std::cout << "Running scenario: "
                  << scenario.name
                  << "...\n";

        ScenarioResult result = run_scenario(scenario);

        results.push_back(result);

        std::cout << "Submitted:   "
                  << result.submitted << "\n";

        std::cout << "Accepted:    "
                  << result.accepted << "\n";

        std::cout << "Rejected:    "
                  << result.rejected << "\n";

        std::cout << "Processed:   "
                  << result.processed << "\n";

        std::cout << std::fixed
                  << std::setprecision(2);

        std::cout << "Throughput:  "
                  << result.throughput_eps
                  << " events/sec\n";

        std::cout << "P50:         "
                  << result.latency.p50_us
                  << " us\n";

        std::cout << "P95:         "
                  << result.latency.p95_us
                  << " us\n";

        std::cout << "P99:         "
                  << result.latency.p99_us
                  << " us\n";

        std::cout << "P99.9:       "
                  << result.latency.p999_us
                  << " us\n";

        std::cout << "Max:         "
                  << result.latency.max_us
                  << " us\n";

        std::cout << "---------------------------------------------\n";
    }

    std::ofstream csv(
        ".\\benchmarks\\results\\latency_under_load.csv"
    );

    csv << "scenario,submitted,accepted,rejected,processed,"
           "throughput_eps,p50_us,p95_us,p99_us,p99_9_us,max_us\n";

    for (const auto& result : results) {
        csv << result.name << ","
            << result.submitted << ","
            << result.accepted << ","
            << result.rejected << ","
            << result.processed << ","
            << std::fixed << std::setprecision(4)
            << result.throughput_eps << ","
            << result.latency.p50_us << ","
            << result.latency.p95_us << ","
            << result.latency.p99_us << ","
            << result.latency.p999_us << ","
            << result.latency.max_us << "\n";
    }

    csv.close();

    bool integrity_passed = true;

    for (const auto& result : results) {
        if (result.processed != result.accepted ||
            result.accepted + result.rejected != result.submitted) {
            integrity_passed = false;
        }
    }

    std::cout << "\n=============================================\n";
    std::cout << "Integrity: "
              << (integrity_passed ? "PASS" : "FAIL")
              << "\n";

    std::cout << "Results written to:\n";
    std::cout << "benchmarks/results/latency_under_load.csv\n";
    std::cout << "=============================================\n";

    return integrity_passed ? 0 : 1;
}

