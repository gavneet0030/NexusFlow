#include "core/event/event.hpp"
#include "core/workers/worker_pool.hpp"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <string>
#include <vector>


using namespace nexusflow;


/*
 * Get monotonic timestamp in nanoseconds.
 */
static std::uint64_t now_ns() {

    return static_cast<std::uint64_t>(
        std::chrono::duration_cast<
            std::chrono::nanoseconds
        >(
            std::chrono::steady_clock::now().time_since_epoch()
        ).count()
    );
}


/*
 * Calculate percentile from sorted latency samples.
 *
 * percentile:
 *
 * 0.50   = P50
 * 0.95   = P95
 * 0.99   = P99
 * 0.999  = P99.9
 */
static double percentile_us(
    const std::vector<std::uint64_t>& sorted_latencies,
    double percentile
) {

    if (sorted_latencies.empty()) {

        return 0.0;
    }


    const double index =
        percentile *
        static_cast<double>(
            sorted_latencies.size() - 1
        );


    const auto lower =
        static_cast<std::size_t>(index);


    const auto upper =
        std::min(
            lower + 1,
            sorted_latencies.size() - 1
        );


    const double fraction =
        index -
        static_cast<double>(lower);


    const double value_ns =
        static_cast<double>(
            sorted_latencies[lower]
        )
        +
        fraction *
        (
            static_cast<double>(
                sorted_latencies[upper]
            )
            -
            static_cast<double>(
                sorted_latencies[lower]
            )
        );


    return value_ns / 1000.0;
}


struct BenchmarkResult {

    std::size_t workers{0};

    std::uint64_t events{0};

    double elapsed_seconds{0.0};

    double throughput{0.0};

    double average_latency_us{0.0};

    double p50_us{0.0};

    double p95_us{0.0};

    double p99_us{0.0};

    double p999_us{0.0};

    double max_latency_us{0.0};

    double checksum{0.0};
};


/*
 * Execute one complete benchmark.
 */
static BenchmarkResult run_benchmark(
    std::size_t worker_count,
    std::size_t event_count
) {

    constexpr std::size_t QUEUE_CAPACITY = 10'000;


    WorkerPool pool(
        worker_count,
        QUEUE_CAPACITY
    );


    pool.start();


    const auto benchmark_start =
        std::chrono::steady_clock::now();


    for (
        std::size_t i = 0;
        i < event_count;
        ++i
    ) {

        Event event;


        event.id =
            static_cast<std::uint64_t>(i + 1);


        /*
         * Timestamp is captured immediately before
         * entering the processing pipeline.
         */
        event.timestamp_ns = now_ns();


        /*
         * Create a realistic priority distribution.
         */
        if (i % 1000 == 0) {

            event.priority =
                EventPriority::CRITICAL;

        }
        else if (i % 100 == 0) {

            event.priority =
                EventPriority::HIGH;

        }
        else {

            event.priority =
                EventPriority::NORMAL;
        }


        event.value =
            static_cast<double>(i % 10000) / 100.0;


        event.source =
            "synthetic";


        event.type =
            "transaction";


        if (!pool.submit(std::move(event))) {

            throw std::runtime_error(
                "Failed to submit event."
            );
        }
    }


    /*
     * Wait until every event is processed.
     */
    pool.shutdown();


    const auto benchmark_end =
        std::chrono::steady_clock::now();


    const double elapsed_seconds =
        std::chrono::duration<double>(
            benchmark_end -
            benchmark_start
        ).count();


    auto latencies =
        pool.collect_latencies();


    if (latencies.size() != event_count) {

        throw std::runtime_error(
            "Latency sample count does not match event count."
        );
    }


    std::sort(
        latencies.begin(),
        latencies.end()
    );


    BenchmarkResult result;


    result.workers =
        worker_count;


    result.events =
        event_count;


    result.elapsed_seconds =
        elapsed_seconds;


    result.throughput =
        static_cast<double>(event_count)
        /
        elapsed_seconds;


    result.average_latency_us =
        pool.average_latency_us();


    result.p50_us =
        percentile_us(
            latencies,
            0.50
        );


    result.p95_us =
        percentile_us(
            latencies,
            0.95
        );


    result.p99_us =
        percentile_us(
            latencies,
            0.99
        );


    result.p999_us =
        percentile_us(
            latencies,
            0.999
        );


    result.max_latency_us =
        static_cast<double>(
            latencies.back()
        )
        /
        1000.0;


    result.checksum =
        pool.checksum();


    return result;
}


/*
 * Print one benchmark result.
 */
static void print_result(
    const BenchmarkResult& result
) {

    std::cout
        << std::fixed
        << std::setprecision(3);


    std::cout
        << "Workers       : "
        << result.workers
        << "\n";


    std::cout
        << "Events        : "
        << result.events
        << "\n";


    std::cout
        << "Time          : "
        << result.elapsed_seconds
        << " sec\n";


    std::cout
        << "Throughput    : "
        << result.throughput
        << " events/sec\n";


    std::cout
        << "Average       : "
        << result.average_latency_us
        << " us\n";


    std::cout
        << "P50           : "
        << result.p50_us
        << " us\n";


    std::cout
        << "P95           : "
        << result.p95_us
        << " us\n";


    std::cout
        << "P99           : "
        << result.p99_us
        << " us\n";


    std::cout
        << "P99.9         : "
        << result.p999_us
        << " us\n";


    std::cout
        << "Max           : "
        << result.max_latency_us
        << " us\n";


    std::cout
        << "Checksum      : "
        << result.checksum
        << "\n";
}


/*
 * Write CSV for Python analysis.
 */
static void write_csv(
    const std::vector<BenchmarkResult>& results,
    const std::string& filename
) {

    std::ofstream file(filename);


    if (!file.is_open()) {

        throw std::runtime_error(
            "Could not open benchmark CSV."
        );
    }


    file
        << "workers,"
        << "events,"
        << "elapsed_seconds,"
        << "throughput_events_sec,"
        << "average_latency_us,"
        << "p50_us,"
        << "p95_us,"
        << "p99_us,"
        << "p99_9_us,"
        << "max_latency_us,"
        << "checksum\n";


    file
        << std::fixed
        << std::setprecision(6);


    for (const auto& result : results) {

        file
            << result.workers << ","
            << result.events << ","
            << result.elapsed_seconds << ","
            << result.throughput << ","
            << result.average_latency_us << ","
            << result.p50_us << ","
            << result.p95_us << ","
            << result.p99_us << ","
            << result.p999_us << ","
            << result.max_latency_us << ","
            << result.checksum
            << "\n";
    }
}


int main() {

    constexpr std::size_t EVENT_COUNT = 100'000;


    /*
     * These are the worker configurations we want to compare.
     */
    const std::vector<std::size_t> worker_counts = {
        1,
        2,
        4,
        8,
        16
    };


    std::cout << "\n";
    std::cout
        << "============================================================\n";

    std::cout
        << "        NEXUSFLOW WORKER SCALING BENCHMARK\n";

    std::cout
        << "============================================================\n";


    std::cout
        << "Events per run : "
        << EVENT_COUNT
        << "\n";


    std::cout
        << "Configurations : 1, 2, 4, 8, 16 workers\n";


    std::cout
        << "============================================================\n";


    std::vector<BenchmarkResult> results;


    for (const auto workers : worker_counts) {

        std::cout << "\n";
        std::cout
            << "------------------------------------------------------------\n";

        std::cout
            << "Running benchmark with "
            << workers
            << " worker(s)...\n";

        std::cout
            << "------------------------------------------------------------\n";


        const auto result =
            run_benchmark(
                workers,
                EVENT_COUNT
            );


        print_result(result);


        results.push_back(result);
    }


    /*
     * Save machine-readable benchmark results.
     */
    write_csv(
        results,
        "benchmarks/results/worker_scaling.csv"
    );


    std::cout << "\n";
    std::cout
        << "============================================================\n";

    std::cout
        << "                 SCALING SUMMARY\n";

    std::cout
        << "============================================================\n";


    std::cout
        << std::left
        << std::setw(10)
        << "Workers"
        << std::setw(20)
        << "Throughput"
        << std::setw(15)
        << "P50(us)"
        << std::setw(15)
        << "P95(us)"
        << std::setw(15)
        << "P99(us)"
        << "\n";


    std::cout
        << "------------------------------------------------------------\n";


    std::cout
        << std::fixed
        << std::setprecision(2);


    for (const auto& result : results) {

        std::cout
            << std::left
            << std::setw(10)
            << result.workers
            << std::setw(20)
            << result.throughput
            << std::setw(15)
            << result.p50_us
            << std::setw(15)
            << result.p95_us
            << std::setw(15)
            << result.p99_us
            << "\n";
    }


    std::cout
        << "\nResults saved to:\n"
        << "benchmarks/results/worker_scaling.csv\n";


    std::cout
        << "\nSTATUS: SCALING BENCHMARK COMPLETE\n";


    return 0;
}
