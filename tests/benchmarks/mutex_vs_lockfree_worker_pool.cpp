#include "core/event/event.hpp"
#include "core/workers/worker_pool.hpp"
#include "core/workers/lock_free_worker_pool.hpp"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

namespace {

using Clock = std::chrono::steady_clock;

struct BenchmarkResult {
    std::string implementation;
    std::size_t workers{0};
    std::size_t events{0};
    double elapsed_seconds{0.0};
    double throughput_events_sec{0.0};
    double p50_us{0.0};
    double p95_us{0.0};
    double p99_us{0.0};
    double max_us{0.0};
    std::size_t processed{0};
    double checksum{0.0};
};

double percentile_us(
    std::vector<double> values,
    double percentile
) {
    if (values.empty()) {
        return 0.0;
    }

    std::sort(values.begin(), values.end());

    const double rank =
        percentile * static_cast<double>(values.size() - 1);

    const std::size_t lower =
        static_cast<std::size_t>(rank);

    const std::size_t upper =
        std::min(
            lower + 1,
            values.size() - 1
        );

    const double fraction =
        rank - static_cast<double>(lower);

    return
        values[lower] +
        fraction *
        (values[upper] - values[lower]);
}

nexusflow::Event make_event(std::size_t index) {

    nexusflow::Event event;

    event.id =
        static_cast<std::uint64_t>(index + 1);

    event.timestamp_ns =
        static_cast<std::uint64_t>(
            std::chrono::duration_cast<
                std::chrono::nanoseconds
            >(
                Clock::now().time_since_epoch()
            ).count()
        );

    event.value =
        1000.0 +
        static_cast<double>(index % 10000);

    event.priority =
        nexusflow::EventPriority::NORMAL;

    event.source = "ab_benchmark";
    event.type = "benchmark_event";

    return event;
}

void print_result(
    const BenchmarkResult& result
) {
    std::cout
        << std::left
        << std::setw(14)
        << result.implementation
        << std::right
        << std::setw(8)
        << result.workers
        << std::setw(18)
        << std::fixed
        << std::setprecision(2)
        << result.throughput_events_sec
        << std::setw(15)
        << result.p50_us
        << std::setw(15)
        << result.p95_us
        << std::setw(15)
        << result.p99_us
        << std::setw(15)
        << result.max_us
        << std::setw(14)
        << result.processed
        << '\n';
}

BenchmarkResult run_mutex(
    std::size_t workers,
    std::size_t event_count
) {
    nexusflow::WorkerPool pool(
        workers,
        event_count
    );

    pool.start();

    const auto start = Clock::now();

    for (std::size_t i = 0; i < event_count; ++i) {
        while (!pool.submit(make_event(i))) {
            std::this_thread::yield();
        }
    }

    pool.shutdown();

    const auto end = Clock::now();

    BenchmarkResult result;

    result.implementation = "mutex";
    result.workers = workers;
    result.events = event_count;

    result.elapsed_seconds =
        std::chrono::duration<double>(
            end - start
        ).count();

    result.throughput_events_sec =
        static_cast<double>(event_count) /
        result.elapsed_seconds;

    result.processed =
        pool.processed();

    result.checksum =
        pool.checksum();

    /*
     * WorkerPool already records latency samples internally.
     * Convert the stored nanosecond samples into microseconds.
     */
    const auto& samples =
        pool.collect_latencies();

    std::vector<double> latency_us;
    latency_us.reserve(samples.size());

    for (const auto ns : samples) {
        latency_us.push_back(
            static_cast<double>(ns) / 1000.0
        );
    }

    result.p50_us =
        percentile_us(latency_us, 0.50);

    result.p95_us =
        percentile_us(latency_us, 0.95);

    result.p99_us =
        percentile_us(latency_us, 0.99);

    result.max_us =
        latency_us.empty()
            ? 0.0
            : *std::max_element(
                latency_us.begin(),
                latency_us.end()
            );

    return result;
}

BenchmarkResult run_lock_free(
    std::size_t workers,
    std::size_t event_count
) {
    nexusflow::LockFreeWorkerPool pool(
        workers,
        event_count
    );

    pool.start();

    const auto start = Clock::now();

    for (std::size_t i = 0; i < event_count; ++i) {

        while (!pool.submit(make_event(i))) {
            std::this_thread::yield();
        }
    }

    while (
        pool.processed() <
        event_count
    ) {
        std::this_thread::yield();
    }

    const auto end = Clock::now();

    pool.shutdown();

    BenchmarkResult result;

    result.implementation = "lock_free";
    result.workers = workers;
    result.events = event_count;

    result.elapsed_seconds =
        std::chrono::duration<double>(
            end - start
        ).count();

    result.throughput_events_sec =
        static_cast<double>(event_count) /
        result.elapsed_seconds;

    result.processed =
        pool.processed();

    result.checksum =
        pool.checksum();

    /*
     * The lock-free pool now exposes the same latency
     * sample interface as the mutex pool.
     */
    const auto& samples =
        pool.collect_latencies();

    std::vector<double> latency_us;
    latency_us.reserve(samples.size());

    for (const auto ns : samples) {
        latency_us.push_back(
            static_cast<double>(ns) / 1000.0
        );
    }

    result.p50_us =
        percentile_us(latency_us, 0.50);

    result.p95_us =
        percentile_us(latency_us, 0.95);

    result.p99_us =
        percentile_us(latency_us, 0.99);

    result.max_us =
        latency_us.empty()
            ? 0.0
            : *std::max_element(
                latency_us.begin(),
                latency_us.end()
            );

    return result;
}

} // namespace

int main() {

    constexpr std::size_t event_count =
        100000;

    const std::vector<std::size_t> worker_counts{
        1,
        2,
        4,
        8,
        16
    };

    std::vector<BenchmarkResult> results;

    results.reserve(
        worker_counts.size() * 2
    );

    std::cout
        << "============================================================\n"
        << "NEXUSFLOW MUTEX VS LOCK-FREE WORKER POOL\n"
        << "============================================================\n"
        << "Events per run: "
        << event_count
        << "\n\n";

    std::cout
        << std::left
        << std::setw(14)
        << "Implementation"
        << std::right
        << std::setw(8)
        << "Workers"
        << std::setw(18)
        << "Throughput"
        << std::setw(15)
        << "P50(us)"
        << std::setw(15)
        << "P95(us)"
        << std::setw(15)
        << "P99(us)"
        << std::setw(15)
        << "Max(us)"
        << std::setw(14)
        << "Processed"
        << "\n";

    std::cout
        << "--------------------------------------------------------------------------------------------------------\n";

    for (const auto workers : worker_counts) {

        auto mutex_result =
            run_mutex(
                workers,
                event_count
            );

        print_result(mutex_result);
        results.push_back(mutex_result);

        auto lock_free_result =
            run_lock_free(
                workers,
                event_count
            );

        print_result(lock_free_result);
        results.push_back(lock_free_result);
    }

    const std::string output_path =
        "benchmarks/results/mutex_vs_lockfree.csv";

    std::ofstream output(
        output_path,
        std::ios::trunc
    );

    if (!output) {
        std::cerr
            << "FAILED TO OPEN RESULT FILE\n";
        return 1;
    }

    output
        << "implementation,"
        << "workers,"
        << "events,"
        << "elapsed_seconds,"
        << "throughput_events_sec,"
        << "p50_us,"
        << "p95_us,"
        << "p99_us,"
        << "max_us,"
        << "processed,"
        << "checksum\n";

    output << std::setprecision(15);

    for (const auto& result : results) {

        output
            << result.implementation << ','
            << result.workers << ','
            << result.events << ','
            << result.elapsed_seconds << ','
            << result.throughput_events_sec << ','
            << result.p50_us << ','
            << result.p95_us << ','
            << result.p99_us << ','
            << result.max_us << ','
            << result.processed << ','
            << result.checksum
            << '\n';
    }

    output.close();

    std::cout
        << "\n============================================================\n"
        << "RESULTS SAVED\n"
        << output_path
        << "\n============================================================\n";

    return 0;
}


