#include "core/processor/integrated_adaptive_pipeline.hpp"
#include "streaming/event_replay/event_replay.hpp"
using nexusflow::streaming::EventReplay;

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <string>
#include <thread>
#include <vector>

using namespace nexusflow;

enum class ExperimentMode
{
    FIXED_SINGLE,
    FIXED_BATCH_32,
    FIXED_PARALLEL_8,
    ADAPTIVE
};

static const char* mode_name(ExperimentMode mode)
{
    switch (mode)
    {
        case ExperimentMode::FIXED_SINGLE:
            return "FIXED_SINGLE";

        case ExperimentMode::FIXED_BATCH_32:
            return "FIXED_BATCH_32";

        case ExperimentMode::FIXED_PARALLEL_8:
            return "FIXED_PARALLEL_8";

        case ExperimentMode::ADAPTIVE:
            return "ADAPTIVE";
    }

    return "UNKNOWN";
}

static double percentile(
    std::vector<double> values,
    double p
)
{
    if (values.empty())
    {
        return 0.0;
    }

    std::sort(values.begin(), values.end());

    const double index =
        (p / 100.0) *
        static_cast<double>(values.size() - 1);

    const auto lower =
        static_cast<std::size_t>(std::floor(index));

    const auto upper =
        static_cast<std::size_t>(std::ceil(index));

    if (lower == upper)
    {
        return values[lower];
    }

    const double fraction =
        index - static_cast<double>(lower);

    return values[lower] +
           (values[upper] - values[lower]) *
           fraction;
}

static void write_header(std::ofstream& out)
{
    out
        << "run_id,"
        << "mode,"
        << "target_rate_eps,"
        << "repetition,"
        << "submitted,"
        << "accepted,"
        << "processed,"
        << "rejected,"
        << "throttled,"
        << "elapsed_seconds,"
        << "throughput_eps,"
        << "average_processing_latency_us,"
        << "p50_processing_latency_us,"
        << "p95_processing_latency_us,"
        << "p99_processing_latency_us,"
        << "p999_processing_latency_us,"
        << "max_processing_latency_us,"
        << "queue_depth,"
        << "workers,"
        << "checksum,"
        << "integrity_pass\n";
}

static SchedulerConfig make_scheduler_config()
{
    SchedulerConfig config;

    config.soft_queue_limit = 256;
    config.hard_queue_limit = 4096;

    config.low_queue_threshold = 8;
    config.worker_queue_threshold = 32;
    config.high_worker_queue_threshold = 128;
    config.extreme_worker_queue_threshold = 512;

    config.low_arrival_rate_eps = 1000.0;
    config.moderate_arrival_rate_eps = 5000.0;

    config.latency_critical_sla_us = 1000;
    config.tail_guard_sla_us = 250;
    config.tail_guard_queue_threshold = 32;

    config.high_priority_batch_size = 4;
    config.micro_batch_size = 32;
    config.parallel_batch_size = 64;

    config.critical_min_workers = 4;
    config.high_priority_min_workers = 2;
    config.latency_critical_min_workers = 2;

    return config;
}

static void configure_mode(
    IntegratedAdaptivePipeline& pipeline,
    ExperimentMode mode
)
{
    switch (mode)
    {
        case ExperimentMode::FIXED_SINGLE:
            pipeline.set_fixed_mode(std::string("FIXED_SINGLE"));
            break;

        case ExperimentMode::FIXED_BATCH_32:
            pipeline.set_fixed_mode(std::string("FIXED_BATCH_32"));
            break;

        case ExperimentMode::FIXED_PARALLEL_8:
            pipeline.set_fixed_mode(std::string("FIXED_PARALLEL_8"));
            break;

        case ExperimentMode::ADAPTIVE:
            pipeline.set_fixed_mode(std::string("ADAPTIVE"));
            break;
    }
}

int main()
{
    const std::string dataset_path =
        "data/processed/nexusflow_events.csv";

    const std::string output_path =
        "benchmarks/results/comparative_scheduler/"
        "comparative_raw.csv";

    const std::vector<std::uint64_t> target_rates =
    {
        10000,
        20000,
        30000,
        40000,
        50000
    };

    const std::vector<ExperimentMode> modes =
    {
        ExperimentMode::FIXED_SINGLE,
        ExperimentMode::FIXED_BATCH_32,
        ExperimentMode::FIXED_PARALLEL_8,
        ExperimentMode::ADAPTIVE
    };

    constexpr int repetitions = 3;
    constexpr std::size_t events_per_run = 10000;

    EventReplay replay;

    if (!replay.load_csv(dataset_path))
    {
        std::cerr
            << "Failed to load dataset: "
            << dataset_path
            << '\n';

        return 1;
    }

    if (replay.size() < events_per_run)
    {
        std::cerr
            << "Dataset does not contain enough events.\n";

        return 1;
    }

    std::ofstream out(
        output_path,
        std::ios::out | std::ios::trunc
    );

    if (!out)
    {
        std::cerr
            << "Failed to open output file: "
            << output_path
            << '\n';

        return 1;
    }

    write_header(out);

    int run_id = 1;

    for (const auto mode : modes)
    {
        for (const auto target_rate : target_rates)
        {
            for (int repetition = 1;
                 repetition <= repetitions;
                 ++repetition)
            {
                std::cout
                    << "\n========================================\n"
                    << "Mode: "
                    << mode_name(mode)
                    << "\nTarget rate: "
                    << target_rate
                    << " EPS\nRepetition: "
                    << repetition
                    << "/"
                    << repetitions
                    << "\n========================================\n";

                const SchedulerConfig config =
                    make_scheduler_config();

                IntegratedAdaptivePipeline pipeline(
                    4096,
                    16,
                    config
                );

                configure_mode(
                    pipeline,
                    mode
                );

                pipeline.start();

                const auto start =
                    std::chrono::steady_clock::now();

                const double interval_us =
                    1'000'000.0 /
                    static_cast<double>(target_rate);

                double next_event_time_us = 0.0;

                std::size_t accepted = 0;
                std::size_t rejected = 0;
                std::size_t throttled = 0;

                std::uint64_t checksum = 0;

                for (std::size_t i = 0;
                     i < events_per_run;
                     ++i)
                {
                    Event event =
                        replay.make_event(i);

                    event.priority =
                        EventPriority::NORMAL;

                    if (mode ==
                        ExperimentMode::FIXED_PARALLEL_8)
                    {
                        event.priority =
                            EventPriority::HIGH;
                    }

                    const bool submitted =
                        pipeline.submit(event);

                    if (submitted)
                    {
                        ++accepted;
                    }
                    else
                    {
                        ++rejected;
                        ++throttled;
                    }

                    checksum += event.id;

                    const auto now =
                        std::chrono::steady_clock::now();

                    const double elapsed_us =
                        static_cast<double>(
                            std::chrono::duration_cast<
                                std::chrono::microseconds
                            >(now - start).count()
                        );

                    if (elapsed_us <
                        next_event_time_us)
                    {
                        const double remaining_us =
                            next_event_time_us -
                            elapsed_us;

                        if (remaining_us > 50.0)
                        {
                            std::this_thread::sleep_for(
                                std::chrono::microseconds(
                                    static_cast<std::int64_t>(
                                        remaining_us
                                    )
                                )
                            );
                        }
                    }

                    next_event_time_us +=
                        interval_us;
                }

                pipeline.drain();

                const auto end =
                    std::chrono::steady_clock::now();

                const double elapsed_seconds =
                    static_cast<double>(
                        std::chrono::duration_cast<
                            std::chrono::microseconds
                        >(end - start).count()
                    ) /
                    1'000'000.0;

                const PipelineMetrics metrics =
                    pipeline.metrics();

                const std::vector<double>
                    latency_samples =
                        pipeline.latency_samples_us();

                const double throughput =
                    elapsed_seconds > 0.0
                        ? static_cast<double>(
                              metrics.processed
                          ) /
                          elapsed_seconds
                        : 0.0;

                const double average_latency =
                    latency_samples.empty()
                        ? 0.0
                        : std::accumulate(
                              latency_samples.begin(),
                              latency_samples.end(),
                              0.0
                          ) /
                          static_cast<double>(
                              latency_samples.size()
                          );

                const double p50 =
                    percentile(
                        latency_samples,
                        50.0
                    );

                const double p95 =
                    percentile(
                        latency_samples,
                        95.0
                    );

                const double p99 =
                    percentile(
                        latency_samples,
                        99.0
                    );

                const double p999 =
                    percentile(
                        latency_samples,
                        99.9
                    );

                const double max_latency =
                    latency_samples.empty()
                        ? 0.0
                        : *std::max_element(
                              latency_samples.begin(),
                              latency_samples.end()
                          );

                const bool integrity =
                    metrics.submitted ==
                        events_per_run &&
                    metrics.accepted ==
                        events_per_run &&
                    metrics.processed ==
                        events_per_run &&
                    metrics.rejected == 0 &&
                    metrics.queue_depth == 0;

                out
                    << run_id << ','
                    << mode_name(mode) << ','
                    << target_rate << ','
                    << repetition << ','
                    << metrics.submitted << ','
                    << metrics.accepted << ','
                    << metrics.processed << ','
                    << metrics.rejected << ','
                    << metrics.throttled << ','
                    << std::fixed
                    << std::setprecision(6)
                    << elapsed_seconds << ','
                    << throughput << ','
                    << average_latency << ','
                    << p50 << ','
                    << p95 << ','
                    << p99 << ','
                    << p999 << ','
                    << max_latency << ','
                    << metrics.queue_depth << ','
                    << metrics.active_workers << ','
                    << metrics.checksum << ','
                    << (integrity
                            ? "PASS"
                            : "FAIL")
                    << '\n';

                out.flush();

                std::cout
                    << "Processed: "
                    << metrics.processed
                    << '\n'
                    << "Throughput: "
                    << throughput
                    << " EPS\n"
                    << "Average processing latency: "
                    << average_latency
                    << " us\n"
                    << "P99: "
                    << p99
                    << " us\n"
                    << "P99.9: "
                    << p999
                    << " us\n"
                    << "Maximum latency: "
                    << max_latency
                    << " us\n"
                    << "Workers: "
                    << metrics.active_workers
                    << '\n'
                    << "Throttled: "
                    << metrics.throttled
                    << '\n'
                    << "Integrity: "
                    << (integrity
                            ? "PASS"
                            : "FAIL")
                    << '\n';

                pipeline.shutdown();

                ++run_id;
            }
        }
    }

    std::cout
        << "\nComparative scheduler experiment completed.\n"
        << "Output: "
        << output_path
        << '\n';

    return 0;
}



