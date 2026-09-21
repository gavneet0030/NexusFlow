#include "core/processor/integrated_adaptive_pipeline.hpp"
#include "core/scheduler/adaptive_scheduler.hpp"

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

using nexusflow::Event;
using nexusflow::EventPriority;
using nexusflow::IntegratedAdaptivePipeline;
using nexusflow::SchedulerConfig;

struct ExperimentConfig {
    std::size_t case_id;
    std::size_t micro_batch_size;
    std::uint64_t sla_us;
    std::size_t soft_queue_limit;
};

double calculate_percentile(
    std::vector<double> values,
    double percentile
) {
    if (values.empty()) {
        return 0.0;
    }

    std::sort(values.begin(), values.end());

    if (values.size() == 1) {
        return values.front();
    }

    const double rank =
        (percentile / 100.0) *
        static_cast<double>(values.size() - 1);

    const std::size_t lower =
        static_cast<std::size_t>(rank);

    const std::size_t upper =
        std::min(lower + 1, values.size() - 1);

    const double fraction =
        rank - static_cast<double>(lower);

    return values[lower] +
           fraction * (values[upper] - values[lower]);
}

struct RunResult {
    std::size_t case_id;
    std::size_t micro_batch_size;
    std::uint64_t sla_us;
    std::size_t soft_queue_limit;

    std::uint64_t submitted;
    std::uint64_t accepted;
    std::uint64_t processed;
    std::uint64_t rejected;
    std::uint64_t throttled;

    double throughput_eps;
    double average_latency_us;
    double p50_latency_us;
    double p95_latency_us;
    double p99_latency_us;
    double p999_latency_us;
    double max_latency_us;

    std::size_t final_queue_depth;
    std::size_t active_workers;
};

Event make_event(std::uint64_t id) {
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

    event.priority = EventPriority::NORMAL;
    event.value = static_cast<double>(id);
    event.source = "threshold_experiment";
    event.type = "benchmark";

    return event;
}

std::vector<ExperimentConfig> build_configurations() {
    const std::size_t batch_sizes[] = {
        8, 16, 32, 64
    };

    const std::uint64_t sla_values[] = {
        500, 1000, 2000, 5000
    };

    const std::size_t queue_limits[] = {
        16, 32, 64, 128
    };

    std::vector<ExperimentConfig> configurations;

    std::size_t case_id = 0;

    for (const auto batch_size : batch_sizes) {
        for (const auto sla_us : sla_values) {
            for (const auto queue_limit : queue_limits) {
                ++case_id;

                configurations.push_back({
                    case_id,
                    batch_size,
                    sla_us,
                    queue_limit
                });
            }
        }
    }

    return configurations;
}

RunResult run_configuration(
    const ExperimentConfig& experiment
) {
    SchedulerConfig scheduler_config;

    scheduler_config.micro_batch_size =
        experiment.micro_batch_size;

    scheduler_config.latency_critical_sla_us =
        experiment.sla_us;

    scheduler_config.soft_queue_limit =
        experiment.soft_queue_limit;

    scheduler_config.hard_queue_limit =
        experiment.soft_queue_limit * 8;

    constexpr std::size_t event_count = 30000;
    constexpr std::size_t queue_capacity = 4096;
    constexpr std::size_t max_workers = 16;

    IntegratedAdaptivePipeline pipeline(
        queue_capacity,
        max_workers,
        scheduler_config
    );

    pipeline.set_sla_budget_us(
        experiment.sla_us
    );

    pipeline.start();

    std::uint64_t accepted = 0;

    const auto start =
        std::chrono::steady_clock::now();

    for (std::size_t i = 0; i < event_count; ++i) {
        if (pipeline.submit(make_event(i))) {
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

    RunResult result;

    result.case_id = experiment.case_id;
    result.micro_batch_size =
        experiment.micro_batch_size;
    result.sla_us =
        experiment.sla_us;
    result.soft_queue_limit =
        experiment.soft_queue_limit;

    result.submitted = event_count;
    result.accepted = accepted;
    result.processed = metrics.processed;
    result.rejected = metrics.rejected;
    result.throttled = metrics.throttled;

    result.throughput_eps =
        elapsed_seconds > 0.0
            ? static_cast<double>(metrics.processed)
                / elapsed_seconds
            : 0.0;

    result.average_latency_us =
        metrics.average_latency_us;

    result.p50_latency_us =
        calculate_percentile(latency_samples, 50.0);

    result.p95_latency_us =
        calculate_percentile(latency_samples, 95.0);

    result.p99_latency_us =
        calculate_percentile(latency_samples, 99.0);

    result.p999_latency_us =
        calculate_percentile(latency_samples, 99.9);

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

    return result;
}

void write_header(std::ofstream& output) {
    output
        << "case_id,"
        << "micro_batch_size,"
        << "sla_us,"
        << "soft_queue_limit,"
        << "submitted,"
        << "accepted,"
        << "processed,"
        << "rejected,"
        << "throttled,"
        << "throughput_eps,"
        << "average_latency_us,"
        << "p50_latency_us,"
        << "p95_latency_us,"
        << "p99_latency_us,"
        << "p999_latency_us,"
        << "max_latency_us,"
        << "final_queue_depth,"
        << "active_workers\n";
}

void write_result(
    std::ofstream& output,
    const RunResult& result
) {
    output
        << result.case_id << ","
        << result.micro_batch_size << ","
        << result.sla_us << ","
        << result.soft_queue_limit << ","
        << result.submitted << ","
        << result.accepted << ","
        << result.processed << ","
        << result.rejected << ","
        << result.throttled << ","
        << std::fixed
        << std::setprecision(3)
        << result.throughput_eps << ","
        << result.average_latency_us << ","
        << result.p50_latency_us << ","
        << result.p95_latency_us << ","
        << result.p99_latency_us << ","
        << result.p999_latency_us << ","
        << result.max_latency_us << ","
        << result.final_queue_depth << ","
        << result.active_workers
        << "\n";
}

} // namespace

int main() {
    std::cout
        << "============================================\n"
        << "NexusFlow End-to-End Threshold Benchmark\n"
        << "============================================\n";

    const std::string output_path =
        "benchmarks/results/end_to_end_thresholds.csv";

    std::filesystem::create_directories(
        "benchmarks/results"
    );

    std::ofstream output(output_path);

    if (!output.is_open()) {
        std::cerr
            << "ERROR: Could not open output file: "
            << output_path
            << "\n";

        return 1;
    }

    write_header(output);

    const auto configurations =
        build_configurations();

    std::size_t completed = 0;

    for (const auto& configuration : configurations) {
        std::cout
            << "Running configuration "
            << configuration.case_id
            << "/"
            << configurations.size()
            << " | batch="
            << configuration.micro_batch_size
            << " | sla="
            << configuration.sla_us
            << " us | queue="
            << configuration.soft_queue_limit
            << "\n";

        const RunResult result =
            run_configuration(configuration);

        write_result(output, result);
        output.flush();

        ++completed;

        const bool integrity =
            result.accepted == result.processed;

        std::cout
            << "  throughput="
            << std::fixed
            << std::setprecision(2)
            << result.throughput_eps
            << " eps"
            << " | avg_latency="
            << result.average_latency_us
            << " us"
            << " | processed="
            << result.processed
            << " | integrity="
            << (integrity ? "PASS" : "FAIL")
            << "\n";
    }

    output.close();

    std::cout
        << "\n============================================\n"
        << "Completed configurations: "
        << completed
        << "/"
        << configurations.size()
        << "\n"
        << "Results: "
        << output_path
        << "\n"
        << "============================================\n";

    if (completed != configurations.size()) {
        std::cerr
            << "ERROR: Not all configurations completed.\n";

        return 1;
    }

    return 0;
}
