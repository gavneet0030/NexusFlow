#include <algorithm>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "core/processor/integrated_adaptive_pipeline.hpp"

using namespace nexusflow;

struct BenchmarkResult {
    std::string name;
    double elapsed_seconds{0.0};
    double throughput_eps{0.0};
    double average_latency_us{0.0};
    double p50_us{0.0};
    double p95_us{0.0};
    double p99_us{0.0};
    double p999_us{0.0};
    std::uint64_t max_latency_us{0};
    std::uint64_t submitted{0};
    std::uint64_t accepted{0};
    std::uint64_t processed{0};
};

double percentile(
    std::vector<double> values,
    double percentile_value
) {
    if (values.empty()) {
        return 0.0;
    }

    std::sort(values.begin(), values.end());

    const double position =
        (percentile_value / 100.0) *
        static_cast<double>(values.size() - 1);

    const std::size_t lower =
        static_cast<std::size_t>(position);

    const std::size_t upper =
        std::min(lower + 1, values.size() - 1);

    const double fraction =
        position - static_cast<double>(lower);

    return static_cast<double>(values[lower]) +
           fraction *
           (
               static_cast<double>(values[upper]) -
               static_cast<double>(values[lower])
           );
}

Event make_event(std::uint64_t id) {
    Event event;

    event.id = id;
    event.timestamp_ns =
        static_cast<std::uint64_t>(
            std::chrono::duration_cast<
                std::chrono::nanoseconds
            >(
                std::chrono::steady_clock::now().time_since_epoch()
            ).count()
        );

    event.priority = EventPriority::NORMAL;
    event.value = static_cast<double>(id % 1000);
    event.source = "controlled_benchmark";
    event.type = "benchmark_event";

    return event;
}

BenchmarkResult run_adaptive(
    const std::string& name,
    std::size_t event_count
) {
    IntegratedAdaptivePipeline pipeline(4096, 16);

    pipeline.set_sla_budget_us(1000);

    pipeline.start();

    const auto start = std::chrono::steady_clock::now();

    for (std::size_t i = 0; i < event_count; ++i) {

        Event event = make_event(i);

        while (!pipeline.submit(event)) {
            pipeline.update_scheduler();
            std::this_thread::yield();
        }

        if ((i + 1) % 256 == 0) {
            pipeline.update_scheduler();
        }
    }

    pipeline.drain();

    const auto end = std::chrono::steady_clock::now();

    const double elapsed =
        std::chrono::duration<double>(end - start).count();

    const auto metrics = pipeline.metrics();
    const auto samples = pipeline.latency_samples_us();

    BenchmarkResult result;

    result.name = name;
    result.elapsed_seconds = elapsed;
    result.throughput_eps =
        elapsed > 0.0
            ? static_cast<double>(metrics.processed) / elapsed
            : 0.0;

    result.average_latency_us = static_cast<std::uint64_t>(metrics.average_latency_us);
    result.p50_us = percentile(samples, 50.0);
    result.p95_us = percentile(samples, 95.0);
    result.p99_us = percentile(samples, 99.0);
    result.p999_us = percentile(samples, 99.9);

    if (!samples.empty()) {
        result.max_latency_us =
            *std::max_element(
                samples.begin(),
                samples.end()
            );
    }

    result.submitted = metrics.submitted;
    result.accepted = metrics.accepted;
    result.processed = metrics.processed;

    pipeline.shutdown();

    return result;
}

BenchmarkResult run_fixed(
    const std::string& name,
    std::size_t event_count,
    std::size_t workers
) {
    IntegratedAdaptivePipeline pipeline(4096, workers);

    pipeline.start();

    pipeline.set_fixed_mode(name);

    const auto start = std::chrono::steady_clock::now();

    for (std::size_t i = 0; i < event_count; ++i) {

        Event event = make_event(i);

        while (!pipeline.submit(event)) {
            pipeline.update_scheduler();
            std::this_thread::yield();
        }
    }

    pipeline.drain();

    const auto end = std::chrono::steady_clock::now();

    const double elapsed =
        std::chrono::duration<double>(end - start).count();

    const auto metrics = pipeline.metrics();
    const auto samples = pipeline.latency_samples_us();

    BenchmarkResult result;

    result.name = name;
    result.elapsed_seconds = elapsed;
    result.throughput_eps =
        elapsed > 0.0
            ? static_cast<double>(metrics.processed) / elapsed
            : 0.0;

    result.average_latency_us = static_cast<std::uint64_t>(metrics.average_latency_us);
    result.p50_us = percentile(samples, 50.0);
    result.p95_us = percentile(samples, 95.0);
    result.p99_us = percentile(samples, 99.0);
    result.p999_us = percentile(samples, 99.9);

    if (!samples.empty()) {
        result.max_latency_us =
            *std::max_element(
                samples.begin(),
                samples.end()
            );
    }

    result.submitted = metrics.submitted;
    result.accepted = metrics.accepted;
    result.processed = metrics.processed;

    pipeline.shutdown();

    return result;
}

void write_results_csv(const std::vector<BenchmarkResult>& results) {
    const std::string output_path =
        "benchmarks/results/adaptive_vs_fixed_results.csv";

    std::filesystem::create_directories(
        "benchmarks/results"
    );

    std::ofstream file(output_path);

    if (!file.is_open()) {
        std::cerr
            << "Failed to open CSV output: "
            << output_path
            << "\n";
        return;
    }

    file
        << "mode,"
        << "elapsed_seconds,"
        << "throughput_eps,"
        << "average_latency_us,"
        << "p50_us,"
        << "p95_us,"
        << "p99_us,"
        << "p999_us,"
        << "max_latency_us,"
        << "submitted,"
        << "accepted,"
        << "processed"
        << "\n";

    file << std::fixed << std::setprecision(6);

    for (const auto& result : results) {
        file
            << result.name << ","
            << result.elapsed_seconds << ","
            << result.throughput_eps << ","
            << result.average_latency_us << ","
            << result.p50_us << ","
            << result.p95_us << ","
            << result.p99_us << ","
            << result.p999_us << ","
            << result.max_latency_us << ","
            << result.submitted << ","
            << result.accepted << ","
            << result.processed
            << "\n";
    }

    file.close();

    std::cout
        << "\nCSV results written to: "
        << output_path
        << "\n";
}
void print_result(const BenchmarkResult& result) {
    std::cout
        << std::left
        << std::setw(22) << result.name
        << std::setw(14) << result.throughput_eps
        << std::setw(14) << result.p50_us
        << std::setw(14) << result.p95_us
        << std::setw(14) << result.p99_us
        << std::setw(14) << result.p999_us
        << std::setw(14) << result.max_latency_us
        << std::setw(12) << result.processed
        << "\n";
}

int main() {
    constexpr std::size_t event_count = 30000;

    std::cout << "============================================\n";
    std::cout << "NexusFlow Adaptive vs Fixed Benchmark\n";
    std::cout << "============================================\n\n";

    std::cout
        << std::left
        << std::setw(22) << "Mode"
        << std::setw(14) << "Throughput"
        << std::setw(14) << "P50 us"
        << std::setw(14) << "P95 us"
        << std::setw(14) << "P99 us"
        << std::setw(14) << "P99.9 us"
        << std::setw(14) << "Max us"
        << std::setw(12) << "Processed"
        << "\n";

    std::cout
        << "------------------------------------------------------------------------------------------------\n";

    std::vector<BenchmarkResult> results;

    results.push_back(
        run_fixed(
            "FIXED_SINGLE",
            event_count,
            1
        )
    );

    results.push_back(
        run_fixed(
            "FIXED_BATCH_32",
            event_count,
            1
        )
    );

    results.push_back(
        run_fixed(
            "FIXED_PARALLEL_8",
            event_count,
            8
        )
    );

    results.push_back(
        run_adaptive(
            "ADAPTIVE",
            event_count
        )
    );

    for (const auto& result : results) {
        print_result(result);
    }

    write_results_csv(results);

    std::cout << "\n============================================\n";
    std::cout << "Integrity Check\n";
    std::cout << "============================================\n";

    bool integrity_pass = true;

    for (const auto& result : results) {
        const bool pass =
    result.accepted == event_count &&
    result.processed == event_count;
            std::cout
            << std::left
            << std::setw(22)
            << result.name
            << (pass ? "PASS" : "FAIL")
            << "\n";

        if (!pass) {
            integrity_pass = false;
        }
    }

    std::cout << "\nOverall Integrity: "
              << (integrity_pass ? "PASS" : "FAIL")
              << "\n";

    std::cout << "============================================\n";

    return integrity_pass ? 0 : 1;
}







