#include "core/event/event.hpp"
#include "core/queue/bounded_mpmc_queue.hpp"
#include "core/queue/lock_free_mpmc_queue.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

using namespace nexusflow;

struct LatencyStats {
    double p50{0.0};
    double p95{0.0};
    double p99{0.0};
    double p999{0.0};
    double maximum{0.0};
};

double percentile(
    std::vector<uint64_t> values,
    double percentage
)
{
    if (values.empty()) {
        return 0.0;
    }

    std::sort(values.begin(), values.end());

    const double rank =
        (percentage / 100.0) *
        static_cast<double>(values.size() - 1);

    const size_t lower =
        static_cast<size_t>(rank);

    const size_t upper =
        std::min(
            lower + 1,
            values.size() - 1
        );

    const double fraction =
        rank - static_cast<double>(lower);

    return static_cast<double>(values[lower]) +
           fraction *
           static_cast<double>(
               values[upper] - values[lower]
           );
}

LatencyStats calculate_stats(
    const std::vector<uint64_t>& values
)
{
    LatencyStats stats;

    stats.p50 = percentile(values, 50.0);
    stats.p95 = percentile(values, 95.0);
    stats.p99 = percentile(values, 99.0);
    stats.p999 = percentile(values, 99.9);

    if (!values.empty()) {
        stats.maximum =
            static_cast<double>(
                *std::max_element(
                    values.begin(),
                    values.end()
                )
            );
    }

    return stats;
}

struct Result {
    std::string queue_type;
    size_t producers;
    uint64_t submitted;
    uint64_t accepted;
    uint64_t processed;
    double throughput;
    LatencyStats latency;
};

template <typename Queue>
Result run_test(
    const std::string& queue_name,
    size_t producer_count
)
{
    constexpr size_t capacity = 4096;
    constexpr size_t events_per_producer = 25000;

    Queue queue(capacity);

    const uint64_t total_events =
        static_cast<uint64_t>(producer_count) *
        static_cast<uint64_t>(events_per_producer);

    std::atomic<uint64_t> submitted{0};
    std::atomic<uint64_t> accepted{0};
    std::atomic<uint64_t> processed{0};

    std::atomic<bool> producers_finished{false};

    std::vector<uint64_t> latencies;
    latencies.reserve(total_events);

    std::mutex latency_mutex;

    std::thread consumer([&]() {
        Event event;

        while (true) {
            if (queue.try_pop(event)) {
                const auto now =
                    std::chrono::steady_clock::now();

                const uint64_t now_ns =
                    static_cast<uint64_t>(
                        std::chrono::duration_cast<
                            std::chrono::nanoseconds
                        >(
                            now.time_since_epoch()
                        ).count()
                    );

                uint64_t latency_us = 0;

                if (now_ns >= event.timestamp_ns) {
                    latency_us =
                        (now_ns - event.timestamp_ns) /
                        1000ULL;
                }

                {
                    std::lock_guard<std::mutex> lock(
                        latency_mutex
                    );

                    latencies.push_back(latency_us);
                }

                volatile uint64_t work = 0;

                for (uint64_t i = 0; i < 50; ++i) {
                    work +=
                        (event.id + i) % 97;
                }

                (void)work;

                processed.fetch_add(
                    1,
                    std::memory_order_relaxed
                );
            }
            else {
                if (
                    producers_finished.load(
                        std::memory_order_acquire
                    ) &&
                    processed.load(
                        std::memory_order_relaxed
                    ) == accepted.load(
                        std::memory_order_relaxed
                    )
                ) {
                    break;
                }

                std::this_thread::yield();
            }
        }
    });

    std::vector<std::thread> producers;
    producers.reserve(producer_count);

    const auto start =
        std::chrono::steady_clock::now();

    for (size_t producer_id = 0;
         producer_id < producer_count;
         ++producer_id) {

        producers.emplace_back(
            [&, producer_id]() {

                for (size_t i = 0;
                     i < events_per_producer;
                     ++i) {

                    const uint64_t event_id =
                        static_cast<uint64_t>(
                            producer_id
                        ) *
                        static_cast<uint64_t>(
                            events_per_producer
                        ) +
                        static_cast<uint64_t>(i);

                    Event event;

                    event.id = event_id;

                    const auto timestamp =
                        std::chrono::steady_clock::now();

                    event.timestamp_ns =
                        static_cast<uint64_t>(
                            std::chrono::duration_cast<
                                std::chrono::nanoseconds
                            >(
                                timestamp.time_since_epoch()
                            ).count()
                        );

                    event.priority =
                        EventPriority::NORMAL;

                    event.value =
                        static_cast<double>(
                            event_id
                        );

                    event.source =
                        "queue-comparison";

                    event.type =
                        "synthetic";

                    submitted.fetch_add(
                        1,
                        std::memory_order_relaxed
                    );

                    while (
                        !queue.try_push(
                            std::move(event)
                        )
                    ) {
                        std::this_thread::yield();
                    }

                    accepted.fetch_add(
                        1,
                        std::memory_order_relaxed
                    );
                }
            }
        );
    }

    for (auto& producer : producers) {
        producer.join();
    }

    producers_finished.store(
        true,
        std::memory_order_release
    );

    consumer.join();

    const auto end =
        std::chrono::steady_clock::now();

    const double elapsed =
        std::chrono::duration<double>(
            end - start
        ).count();

    Result result;

    result.queue_type = queue_name;
    result.producers = producer_count;
    result.submitted = submitted.load();
    result.accepted = accepted.load();
    result.processed = processed.load();

    result.throughput =
        elapsed > 0.0
            ? static_cast<double>(
                  result.processed
              ) / elapsed
            : 0.0;

    result.latency =
        calculate_stats(latencies);

    return result;
}

void print_result(const Result& result)
{
    std::cout
        << std::left
        << std::setw(14)
        << result.queue_type
        << std::setw(10)
        << result.producers
        << std::setw(14)
        << result.throughput
        << std::setw(12)
        << result.latency.p50
        << std::setw(12)
        << result.latency.p95
        << std::setw(12)
        << result.latency.p99
        << std::setw(12)
        << result.latency.p999
        << std::setw(12)
        << result.latency.maximum
        << "\n";
}

int main()
{
    std::vector<Result> results;

    const std::vector<size_t> producer_counts = {
        1, 2, 4, 8
    };

    std::cout
        << "============================================================\n";

    std::cout
        << "NexusFlow Queue Performance Comparison\n";

    std::cout
        << "============================================================\n\n";

    std::cout
        << std::left
        << std::setw(14)
        << "Queue"
        << std::setw(10)
        << "Producers"
        << std::setw(14)
        << "Throughput"
        << std::setw(12)
        << "P50(us)"
        << std::setw(12)
        << "P95(us)"
        << std::setw(12)
        << "P99(us)"
        << std::setw(12)
        << "P99.9(us)"
        << std::setw(12)
        << "Max(us)"
        << "\n";

    std::cout
        << "------------------------------------------------------------\n";

    for (size_t producers : producer_counts) {
        std::cout
            << "Running mutex queue with "
            << producers
            << " producers...\n";

        Result mutex_result =
            run_test<BoundedMPMCQueue<Event>>(
                "MUTEX",
                producers
            );

        print_result(mutex_result);
        results.push_back(mutex_result);

        std::cout
            << "Running lock-free queue with "
            << producers
            << " producers...\n";

        Result lock_free_result =
            run_test<LockFreeMPMCQueue<Event>>(
                "LOCK_FREE",
                producers
            );

        print_result(lock_free_result);
        results.push_back(lock_free_result);

        std::cout << "\n";
    }

    std::ofstream csv(
        ".\\benchmarks\\results\\queue_comparison.csv"
    );

    csv
        << "queue_type,producers,submitted,accepted,"
           "processed,throughput_eps,p50_us,p95_us,"
           "p99_us,p99_9_us,max_us\n";

    for (const auto& result : results) {
        csv
            << result.queue_type << ","
            << result.producers << ","
            << result.submitted << ","
            << result.accepted << ","
            << result.processed << ","
            << std::fixed
            << std::setprecision(4)
            << result.throughput << ","
            << result.latency.p50 << ","
            << result.latency.p95 << ","
            << result.latency.p99 << ","
            << result.latency.p999 << ","
            << result.latency.maximum
            << "\n";
    }

    csv.close();

    bool integrity_passed = true;

    for (const auto& result : results) {
        if (
            result.submitted !=
            result.accepted ||
            result.accepted !=
            result.processed
        ) {
            integrity_passed = false;
        }
    }

    std::cout
        << "============================================================\n";

    std::cout
        << "Integrity: "
        << (
            integrity_passed
                ? "PASS"
                : "FAIL"
        )
        << "\n";

    std::cout
        << "Results written to:\n"
        << "benchmarks/results/queue_comparison.csv\n";

    std::cout
        << "============================================================\n";

    return integrity_passed ? 0 : 1;
}
