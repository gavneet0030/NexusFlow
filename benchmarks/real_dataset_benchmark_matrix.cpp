#include "core/processor/integrated_adaptive_pipeline.hpp"
#include "streaming/event_replay/event_replay.hpp"
using nexusflow::streaming::EventReplay;

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

namespace {

struct BenchmarkResult {
    std::size_t target_rate_eps{0};
    std::size_t event_count{0};

    std::uint64_t submitted{0};
    std::uint64_t accepted{0};
    std::uint64_t processed{0};
    std::uint64_t rejected{0};
    std::uint64_t throttled{0};

    double elapsed_seconds{0.0};
    double throughput_eps{0.0};

    double average_latency_us{0.0};
    double p50_latency_us{0.0};
    double p95_latency_us{0.0};
    double p99_latency_us{0.0};
    double p999_latency_us{0.0};
    double max_latency_us{0.0};

    std::size_t final_queue_depth{0};
    std::size_t active_workers{0};

    std::uint64_t checksum{0};
    bool integrity_pass{false};
};

double percentile(
    std::vector<double> values,
    double percentile_value
) {
    if (values.empty()) {
        return 0.0;
    }

    std::sort(values.begin(), values.end());

    if (values.size() == 1) {
        return values.front();
    }

    const double position =
        percentile_value *
        static_cast<double>(values.size() - 1);

    const auto lower =
        static_cast<std::size_t>(position);

    const auto upper =
        std::min(
            lower + 1,
            values.size() - 1
        );

    const double fraction =
        position -
        static_cast<double>(lower);

    return values[lower] +
        fraction *
        (values[upper] - values[lower]);
}

BenchmarkResult run_benchmark(
    const std::string& input_path,
    std::size_t target_rate_eps,
    std::size_t event_count
) {
    nexusflow::streaming::EventReplay replay;

    if (!replay.load_csv(input_path)) {
        throw std::runtime_error(
            "Failed to load dataset."
        );
    }

    if (replay.empty()) {
        throw std::runtime_error(
            "Dataset contains no events."
        );
    }

    event_count =
        std::min(event_count, replay.size());

    nexusflow::IntegratedAdaptivePipeline pipeline(
        4096,
        16
    );

    pipeline.start();

    std::uint64_t accepted = 0;
    std::uint64_t checksum = 0;

    const auto start =
        std::chrono::steady_clock::now();

    const auto interval =
        target_rate_eps > 0
            ? std::chrono::duration<double>(
                  1.0 /
                  static_cast<double>(
                      target_rate_eps
                  )
              )
            : std::chrono::duration<double>(0.0);

    auto next_emit = start;

    for (std::size_t i = 0; i < event_count; ++i) {

        if (target_rate_eps > 0) {
            next_emit +=
                std::chrono::duration_cast<
                    std::chrono::steady_clock::duration
                >(interval);

            std::this_thread::sleep_until(
                next_emit
            );
        }

        nexusflow::Event event =
            replay.make_event(i);

        checksum += event.id;

        if (pipeline.submit(std::move(event))) {
            ++accepted;
        }
    }

    pipeline.drain();

    const auto end =
        std::chrono::steady_clock::now();

    const double elapsed_seconds =
        std::chrono::duration<double>(
            end - start
        ).count();

    const auto metrics =
        pipeline.metrics();

    const auto latency_samples =
        pipeline.latency_samples_us();

    pipeline.shutdown();

    BenchmarkResult result;

    result.target_rate_eps =
        target_rate_eps;

    result.event_count =
        event_count;

    result.submitted =
        metrics.submitted;

    result.accepted =
        metrics.accepted;

    result.processed =
        metrics.processed;

    result.rejected =
        metrics.rejected;

    result.throttled =
        metrics.throttled;

    result.elapsed_seconds =
        elapsed_seconds;

    result.throughput_eps =
        elapsed_seconds > 0.0
            ? static_cast<double>(
                  metrics.processed
              ) / elapsed_seconds
            : 0.0;

    result.average_latency_us =
        metrics.average_latency_us;

    result.p50_latency_us =
        percentile(
            latency_samples,
            0.50
        );

    result.p95_latency_us =
        percentile(
            latency_samples,
            0.95
        );

    result.p99_latency_us =
        percentile(
            latency_samples,
            0.99
        );

    result.p999_latency_us =
        percentile(
            latency_samples,
            0.999
        );

    result.max_latency_us =
        latency_samples.empty()
            ? 0.0
            : *std::max_element(
                  latency_samples.begin(),
                  latency_samples.end()
              );

    result.final_queue_depth =
        metrics.queue_depth;

    result.active_workers =
        metrics.active_workers;

    result.checksum =
        checksum;

    result.integrity_pass =
        metrics.processed ==
            event_count &&
        accepted ==
            event_count &&
        metrics.rejected ==
            0 &&
        metrics.throttled ==
            0 &&
        metrics.queue_depth ==
            0;

    return result;
}

void write_header(
    std::ofstream& file
) {
    file
        << "target_rate_eps,"
        << "event_count,"
        << "submitted,"
        << "accepted,"
        << "processed,"
        << "rejected,"
        << "throttled,"
        << "elapsed_seconds,"
        << "throughput_eps,"
        << "average_latency_us,"
        << "p50_latency_us,"
        << "p95_latency_us,"
        << "p99_latency_us,"
        << "p999_latency_us,"
        << "max_latency_us,"
        << "final_queue_depth,"
        << "active_workers,"
        << "checksum,"
        << "integrity_pass\n";
}

void write_result(
    std::ofstream& file,
    const BenchmarkResult& result
) {
    file
        << result.target_rate_eps << ","
        << result.event_count << ","
        << result.submitted << ","
        << result.accepted << ","
        << result.processed << ","
        << result.rejected << ","
        << result.throttled << ","
        << std::fixed
        << std::setprecision(6)
        << result.elapsed_seconds << ","
        << result.throughput_eps << ","
        << result.average_latency_us << ","
        << result.p50_latency_us << ","
        << result.p95_latency_us << ","
        << result.p99_latency_us << ","
        << result.p999_latency_us << ","
        << result.max_latency_us << ","
        << result.final_queue_depth << ","
        << result.active_workers << ","
        << result.checksum << ","
        << (result.integrity_pass ? "PASS" : "FAIL")
        << "\n";
}

}

int main() {

    const std::string input_path =
        "data/processed/nexusflow_events.csv";

    const std::string output_path =
        "benchmarks/results/real_dataset_benchmark_matrix.csv";

    std::filesystem::create_directories(
        "benchmarks/results"
    );

    const std::vector<std::size_t> rates = {
        1000,
        10000,
        50000,
        100000
    };

    const std::size_t events_per_test =
        10000;

    std::ofstream output(
        output_path,
        std::ios::trunc
    );

    if (!output.is_open()) {
        std::cerr
            << "Failed to open output file: "
            << output_path
            << "\n";

        return 1;
    }

    write_header(output);

    std::cout
        << "NexusFlow Real Dataset Benchmark Matrix\n"
        << "========================================\n"
        << "Input: "
        << input_path
        << "\n"
        << "Events per test: "
        << events_per_test
        << "\n\n";

    for (const auto rate : rates) {

        std::cout
            << "----------------------------------------\n"
            << "Target rate: "
            << rate
            << " EPS\n"
            << "----------------------------------------\n";

        try {

            const auto result =
                run_benchmark(
                    input_path,
                    rate,
                    events_per_test
                );

            write_result(
                output,
                result
            );

            std::cout
                << "Submitted: "
                << result.submitted
                << "\n";

            std::cout
                << "Accepted: "
                << result.accepted
                << "\n";

            std::cout
                << "Processed: "
                << result.processed
                << "\n";

            std::cout
                << "Throughput: "
                << result.throughput_eps
                << " EPS\n";

            std::cout
                << "Average latency: "
                << result.average_latency_us
                << " us\n";

            std::cout
                << "p50: "
                << result.p50_latency_us
                << " us\n";

            std::cout
                << "p95: "
                << result.p95_latency_us
                << " us\n";

            std::cout
                << "p99: "
                << result.p99_latency_us
                << " us\n";

            std::cout
                << "p99.9: "
                << result.p999_latency_us
                << " us\n";

            std::cout
                << "Max: "
                << result.max_latency_us
                << " us\n";

            std::cout
                << "Workers: "
                << result.active_workers
                << "\n";

            std::cout
                << "Queue depth: "
                << result.final_queue_depth
                << "\n";

            std::cout
                << "Integrity: "
                << (result.integrity_pass
                        ? "PASS"
                        : "FAIL")
                << "\n\n";

            if (!result.integrity_pass) {
                std::cerr
                    << "Benchmark integrity failed.\n";

                return 1;
            }

        }
        catch (const std::exception& error) {

            std::cerr
                << "Benchmark failed: "
                << error.what()
                << "\n";

            return 1;
        }
    }

    output.close();

    std::cout
        << "========================================\n"
        << "REAL DATASET BENCHMARK MATRIX COMPLETE\n"
        << "========================================\n"
        << "\n"
        << "Results:\n"
        << output_path
        << "\n";

    return 0;
}
