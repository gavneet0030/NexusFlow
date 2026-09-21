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
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace {

struct Result {
    int run_id{};
    std::size_t target_rate_eps{};

    std::uint64_t submitted{};
    std::uint64_t accepted{};
    std::uint64_t processed{};
    std::uint64_t rejected{};
    std::uint64_t throttled{};

    double elapsed_seconds{};
    double throughput_eps{};

    double average_latency_us{};
    double p50_us{};
    double p95_us{};
    double p99_us{};
    double p999_us{};
    double max_us{};

    std::size_t queue_depth{};
    std::size_t workers{};

    std::uint64_t checksum{};
    bool integrity_pass{};
};

double percentile(
    std::vector<double> values,
    double p
) {
    if (values.empty()) {
        return 0.0;
    }

    std::sort(values.begin(), values.end());

    if (values.size() == 1) {
        return values.front();
    }

    const double position =
        p * static_cast<double>(values.size() - 1);

    const std::size_t lower =
        static_cast<std::size_t>(position);

    const std::size_t upper =
        std::min(
            lower + 1,
            values.size() - 1
        );

    const double fraction =
        position - static_cast<double>(lower);

    return values[lower] +
        fraction *
        (values[upper] - values[lower]);
}

Result run_once(
    const std::string& input,
    std::size_t rate,
    std::size_t events,
    int run_id
) {
    nexusflow::streaming::EventReplay replay;

    if (!replay.load_csv(input)) {
        throw std::runtime_error(
            "Failed to load dataset."
        );
    }

    if (replay.empty()) {
        throw std::runtime_error(
            "Dataset contains no events."
        );
    }

    events = std::min(
        events,
        replay.size()
    );

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
        std::chrono::duration<double>(
            1.0 /
            static_cast<double>(rate)
        );

    auto next_emit = start;

    for (std::size_t i = 0; i < events; ++i) {

        next_emit +=
            std::chrono::duration_cast<
                std::chrono::steady_clock::duration
            >(interval);

        std::this_thread::sleep_until(
            next_emit
        );

        nexusflow::Event event =
            replay.make_event(i);

        checksum += event.id;

        if (pipeline.submit(
                std::move(event)
            )) {
            ++accepted;
        }
    }

    pipeline.drain();

    const auto end =
        std::chrono::steady_clock::now();

    const double elapsed =
        std::chrono::duration<double>(
            end - start
        ).count();

    const auto metrics =
        pipeline.metrics();

    const auto latency_samples =
        pipeline.latency_samples_us();

    pipeline.shutdown();

    Result result;

    result.run_id = run_id;
    result.target_rate_eps = rate;

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
        elapsed;

    result.throughput_eps =
        elapsed > 0.0
            ? static_cast<double>(
                  metrics.processed
              ) / elapsed
            : 0.0;

    result.average_latency_us =
        metrics.average_latency_us;

    result.p50_us =
        percentile(
            latency_samples,
            0.50
        );

    result.p95_us =
        percentile(
            latency_samples,
            0.95
        );

    result.p99_us =
        percentile(
            latency_samples,
            0.99
        );

    result.p999_us =
        percentile(
            latency_samples,
            0.999
        );

    result.max_us =
        latency_samples.empty()
            ? 0.0
            : *std::max_element(
                  latency_samples.begin(),
                  latency_samples.end()
              );

    result.queue_depth =
        metrics.queue_depth;

    result.workers =
        metrics.active_workers;

    result.checksum =
        checksum;

    result.integrity_pass =
        metrics.submitted == events &&
        metrics.accepted == events &&
        metrics.processed == events &&
        metrics.rejected == 0 &&
        metrics.queue_depth == 0;

    return result;
}

void write_result(
    std::ofstream& out,
    const Result& result
) {
    out
        << result.run_id << ","
        << result.target_rate_eps << ","
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
        << result.p50_us << ","
        << result.p95_us << ","
        << result.p99_us << ","
        << result.p999_us << ","
        << result.max_us << ","
        << result.queue_depth << ","
        << result.workers << ","
        << result.checksum << ","
        << (
            result.integrity_pass
                ? "PASS"
                : "FAIL"
        )
        << '\n';
}

}

int main() {

    const std::string input =
        "data/processed/nexusflow_events.csv";

    const std::string output =
        "benchmarks/results/real_dataset_repeated_raw.csv";

    const std::vector<std::size_t> rates = {
        50000,
        100000
    };

    constexpr int repetitions = 5;

    constexpr std::size_t events_per_run =
        10000;

    std::filesystem::create_directories(
        "benchmarks/results"
    );

    const bool output_exists =
        std::filesystem::exists(output) &&
        std::filesystem::file_size(output) > 0;

    std::ofstream out(
        output,
        std::ios::app
    );

    if (!out.is_open()) {
        std::cerr
            << "Failed to open output file."
            << '\n';

        return 1;
    }

    if (!output_exists) {
        out
            << "run_id,"
        << "target_rate_eps,"
        << "submitted,"
        << "accepted,"
        << "processed,"
        << "rejected,"
        << "throttled,"
        << "elapsed_seconds,"
        << "throughput_eps,"
        << "average_latency_us,"
        << "p50_us,"
        << "p95_us,"
        << "p99_us,"
        << "p999_us,"
        << "max_us,"
        << "queue_depth,"
        << "workers,"
        << "checksum,"
            << "integrity_pass"
            << '\n';
    }

    std::cout
        << "NexusFlow Repeated Real Dataset Experiment"
        << '\n'
        << "==========================================="
        << '\n'
        << "Dataset: "
        << input
        << '\n'
        << "Events per run: "
        << events_per_run
        << '\n'
        << "Repetitions: "
        << repetitions
        << '\n'
        << '\n';

    int run_id = 12;

    for (const auto rate : rates) {

        std::cout
            << "========================================"
            << '\n'
            << "RATE: "
            << rate
            << " EPS"
            << '\n'
            << "========================================"
            << '\n';

        int repetition_start = 1;

        if (rate == 50000) {
            repetition_start = 2;
        }

        for (
            int repetition = repetition_start;
            repetition <= repetitions;
            ++repetition
        ) {

            std::cout
                << "Run "
                << repetition
                << "/"
                << repetitions
                << "..."
                << '\n';

            try {

                const Result result =
                    run_once(
                        input,
                        rate,
                        events_per_run,
                        run_id
                    );

                write_result(
                    out,
                    result
                );

                std::cout
                    << "Throughput: "
                    << result.throughput_eps
                    << " EPS"
                    << '\n';

                std::cout
                    << "p99: "
                    << result.p99_us
                    << " us"
                    << '\n';

                std::cout
                    << "p99.9: "
                    << result.p999_us
                    << " us"
                    << '\n';

                std::cout
                    << "Max: "
                    << result.max_us
                    << " us"
                    << '\n';

                std::cout
                    << "Integrity: "
                    << (
                        result.integrity_pass
                            ? "PASS"
                            : "FAIL"
                    )
                    << '\n'
                    << '\n';

                if (!result.integrity_pass) {
                    std::cerr
                        << "Integrity failure detected."
                        << '\n';

                    return 1;
                }

                ++run_id;
            }
            catch (
                const std::exception& error
            ) {

                std::cerr
                    << "Run failed: "
                    << error.what()
                    << '\n';

                return 1;
            }
        }
    }

    out.close();

    std::cout
        << "========================================"
        << '\n'
        << "REPEATED EXPERIMENT COMPLETE"
        << '\n'
        << "========================================"
        << '\n'
        << '\n'
        << "Results:"
        << '\n'
        << output
        << '\n';

    return 0;
}


