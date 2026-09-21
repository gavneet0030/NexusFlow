#include "core/processor/integrated_adaptive_pipeline.hpp"
#include "streaming/event_replay/event_replay.hpp"
using nexusflow::streaming::EventReplay;

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

using namespace nexusflow;

namespace {

struct Percentiles {
    double p50{0.0};
    double p95{0.0};
    double p99{0.0};
    double p999{0.0};
    double max{0.0};
};

double percentile(
    std::vector<double> values,
    double percentile_value
) {
    if (values.empty()) {
        return 0.0;
    }

    std::sort(values.begin(), values.end());

    const double rank =
        (percentile_value / 100.0) *
        static_cast<double>(values.size() - 1);

    const std::size_t lower =
        static_cast<std::size_t>(rank);

    const std::size_t upper =
        std::min(lower + 1, values.size() - 1);

    const double fraction =
        rank - static_cast<double>(lower);

    return values[lower] +
           fraction *
           (values[upper] - values[lower]);
}

Percentiles calculate_percentiles(
    const std::vector<double>& values
) {
    Percentiles result;

    if (values.empty()) {
        return result;
    }

    result.p50 = percentile(values, 50.0);
    result.p95 = percentile(values, 95.0);
    result.p99 = percentile(values, 99.0);
    result.p999 = percentile(values, 99.9);

    result.max = *std::max_element(
        values.begin(),
        values.end()
    );

    return result;
}

void print_percentiles(
    const std::string& name,
    const Percentiles& p
) {
    std::cout
        << "\n"
        << name << "\n"
        << "  P50:   " << p.p50 << " us\n"
        << "  P95:   " << p.p95 << " us\n"
        << "  P99:   " << p.p99 << " us\n"
        << "  P99.9: " << p.p999 << " us\n"
        << "  Max:   " << p.max << " us\n";
}

bool write_header_if_needed(
    const std::string& output
) {
    namespace fs = std::filesystem;

    const bool exists =
        fs::exists(output);

    const bool empty =
        exists &&
        fs::file_size(output) == 0;

    std::ofstream file(
        output,
        std::ios::app
    );

    if (!file.is_open()) {
        return false;
    }

    if (!exists || empty) {
        file
            << "run_id,target_rate_eps,repetition,"
            << "submitted,accepted,processed,rejected,"
            << "throttled,elapsed_seconds,throughput_eps,"
            << "queue_wait_p50_us,queue_wait_p95_us,"
            << "queue_wait_p99_us,queue_wait_p999_us,"
            << "queue_wait_max_us,"
            << "processing_p50_us,processing_p95_us,"
            << "processing_p99_us,processing_p999_us,"
            << "processing_max_us,"
            << "completion_p50_us,completion_p95_us,"
            << "completion_p99_us,completion_p999_us,"
            << "completion_max_us,"
            << "e2e_p50_us,e2e_p95_us,e2e_p99_us,"
            << "e2e_p999_us,e2e_max_us,"
            << "workers,queue_depth,integrity_pass\n";
    }

    return true;
}

} // namespace

int main() {
    try {
        const std::string dataset =
            "data/processed/nexusflow_events.csv";

        const std::string output =
            "benchmarks/results/real_end_to_end_latency_multi_load_v2_20260908_185246.csv";

        const std::vector<std::uint64_t> rates = {
            1000,
            10000,
            20000,
            30000,
            50000
        };

        constexpr std::size_t repetitions = 3;
        constexpr std::size_t events_per_run = 10000;
        constexpr std::size_t max_workers = 16;

        std::filesystem::create_directories(
            "benchmarks/results"
        );

        if (!write_header_if_needed(output)) {
            std::cerr
                << "ERROR: Cannot open output file.\n";
            return 1;
        }

        EventReplay replay;

        std::cout
            << "========================================\n"
            << "NEXUSFLOW REAL E2E MULTI-LOAD EXPERIMENT\n"
            << "========================================\n\n"
            << "Dataset: " << dataset << "\n"
            << "Events per run: " << events_per_run << "\n"
            << "Repetitions: " << repetitions << "\n"
            << "Maximum workers: " << max_workers << "\n"
            << "Load levels: " << rates.size() << "\n"
            << "Total runs: "
            << rates.size() * repetitions
            << "\n\n";

        std::cout
            << "Loading dataset...\n"
            << std::flush;

        if (!replay.load_csv(dataset)) {
            std::cerr
                << "ERROR: Failed to load dataset.\n";
            return 1;
        }

        if (replay.size() < events_per_run) {
            std::cerr
                << "ERROR: Dataset contains fewer events than required.\n";
            return 1;
        }

        std::cout
            << "Available events: "
            << replay.size()
            << "\n\n";

        int run_id = 1;

        for (const auto target_rate : rates) {
            for (std::size_t repetition = 1;
                 repetition <= repetitions;
                 ++repetition) {

                std::cout
                    << "========================================\n"
                    << "RUN "
                    << run_id
                    << " / "
                    << rates.size() * repetitions
                    << "\n"
                    << "========================================\n"
                    << "Target rate: "
                    << target_rate
                    << " EPS\n"
                    << "Repetition: "
                    << repetition
                    << " / "
                    << repetitions
                    << "\n"
                    << std::flush;

                IntegratedAdaptivePipeline pipeline(
                    4096,
                    max_workers
                );

                pipeline.set_fixed_mode(
                    std::string("ADAPTIVE")
                );

                pipeline.start();

                const auto start =
                    std::chrono::steady_clock::now();

                std::size_t accepted = 0;
                std::size_t submitted = 0;

                const double interval_us =
                    1000000.0 /
                    static_cast<double>(target_rate);

                double next_send_us = 0.0;

                for (std::size_t i = 0;
                     i < events_per_run;
                     ++i) {

                    Event event =
                        replay.make_event(i);

                    ++submitted;

                    if (pipeline.submit(
                            std::move(event)
                        )) {
                        ++accepted;
                    }

                    if ((i + 1) % 1000 == 0) {
                        std::cout
                            << "Progress: "
                            << (i + 1)
                            << "/"
                            << events_per_run
                            << " | Accepted: "
                            << accepted
                            << " | Processed: "
                            << pipeline.metrics().processed
                            << " | Queue: "
                            << pipeline.queue_depth()
                            << " | Workers: "
                            << pipeline.active_workers()
                            << "\n"
                            << std::flush;
                    }

                    next_send_us += interval_us;

                    const auto now =
                        std::chrono::steady_clock::now();

                    const auto elapsed_us =
                        std::chrono::duration<double, std::micro>(
                            now - start
                        ).count();

                    if (next_send_us > elapsed_us) {
                        const auto remaining_us =
                            next_send_us - elapsed_us;

                        if (remaining_us > 1000.0) {
                            std::this_thread::sleep_for(
                                std::chrono::microseconds(
                                    static_cast<long long>(
                                        remaining_us
                                    )
                                )
                            );
                        }
                    }
                }

                std::cout
                    << "\nSubmission complete.\n"
                    << "Accepted: "
                    << accepted
                    << "\n"
                    << "Draining pipeline...\n"
                    << std::flush;

                pipeline.drain();

                const auto end =
                    std::chrono::steady_clock::now();

                const double elapsed_seconds =
                    std::chrono::duration<double>(
                        end - start
                    ).count();

                const auto metrics =
                    pipeline.metrics();

                const auto breakdowns =
                    pipeline.latency_breakdown_samples();

                std::vector<double> queue_wait;
                std::vector<double> processing;
                std::vector<double> completion;
                std::vector<double> e2e;

                queue_wait.reserve(
                    breakdowns.size()
                );

                processing.reserve(
                    breakdowns.size()
                );

                completion.reserve(
                    breakdowns.size()
                );

                e2e.reserve(
                    breakdowns.size()
                );

                for (const auto& sample : breakdowns) {
                    queue_wait.push_back(
                        static_cast<double>(sample.queue_wait_us())
                    );

                    processing.push_back(
                        static_cast<double>(sample.processing_us())
                    );

                    completion.push_back(
                        static_cast<double>(sample.completion_us())
                    );

                    e2e.push_back(
                        static_cast<double>(sample.end_to_end_us())
                    );
                }

                const Percentiles queue_stats =
                    calculate_percentiles(queue_wait);

                const Percentiles processing_stats =
                    calculate_percentiles(processing);

                const Percentiles completion_stats =
                    calculate_percentiles(completion);

                const Percentiles e2e_stats =
                    calculate_percentiles(e2e);

                const double throughput =
                    elapsed_seconds > 0.0
                        ? static_cast<double>(metrics.processed) /
                          elapsed_seconds
                        : 0.0;

                const bool integrity =
                    submitted == events_per_run &&
                    metrics.accepted == submitted &&
                    metrics.processed == submitted &&
                    metrics.rejected == 0 &&
                    pipeline.queue_depth() == 0 &&
                    breakdowns.size() == submitted;

                std::cout
                    << "\nRESULT\n"
                    << "Submitted: "
                    << submitted
                    << "\nAccepted: "
                    << metrics.accepted
                    << "\nProcessed: "
                    << metrics.processed
                    << "\nRejected: "
                    << metrics.rejected
                    << "\nThrottled: "
                    << metrics.throttled
                    << "\nElapsed: "
                    << elapsed_seconds
                    << " s\n"
                    << "Throughput: "
                    << throughput
                    << " EPS\n"
                    << "Workers: "
                    << pipeline.active_workers()
                    << "\n"
                    << "Queue depth: "
                    << pipeline.queue_depth()
                    << "\n";

                print_percentiles(
                    "QUEUE WAIT LATENCY",
                    queue_stats
                );

                print_percentiles(
                    "PROCESSING LATENCY",
                    processing_stats
                );

                print_percentiles(
                    "COMPLETION LATENCY",
                    completion_stats
                );

                print_percentiles(
                    "END-TO-END LATENCY",
                    e2e_stats
                );

                std::ofstream file(
                    output,
                    std::ios::app
                );

                file
                    << run_id << ","
                    << target_rate << ","
                    << repetition << ","
                    << submitted << ","
                    << metrics.accepted << ","
                    << metrics.processed << ","
                    << metrics.rejected << ","
                    << metrics.throttled << ","
                    << std::fixed
                    << std::setprecision(6)
                    << elapsed_seconds << ","
                    << throughput << ","
                    << queue_stats.p50 << ","
                    << queue_stats.p95 << ","
                    << queue_stats.p99 << ","
                    << queue_stats.p999 << ","
                    << queue_stats.max << ","
                    << processing_stats.p50 << ","
                    << processing_stats.p95 << ","
                    << processing_stats.p99 << ","
                    << processing_stats.p999 << ","
                    << processing_stats.max << ","
                    << completion_stats.p50 << ","
                    << completion_stats.p95 << ","
                    << completion_stats.p99 << ","
                    << completion_stats.p999 << ","
                    << completion_stats.max << ","
                    << e2e_stats.p50 << ","
                    << e2e_stats.p95 << ","
                    << e2e_stats.p99 << ","
                    << e2e_stats.p999 << ","
                    << e2e_stats.max << ","
                    << pipeline.active_workers() << ","
                    << pipeline.queue_depth() << ","
                    << (integrity ? 1 : 0)
                    << "\n";

                file.close();

                pipeline.shutdown();

                std::cout
                    << "\nIntegrity: "
                    << (integrity ? "PASS" : "FAIL")
                    << "\n\n";

                if (!integrity) {
                    std::cerr
                        << "ERROR: Integrity failure in run "
                        << run_id
                        << ".\n";
                    return 1;
                }

                ++run_id;
            }
        }

        std::cout
            << "========================================\n"
            << "MULTI-LOAD E2E EXPERIMENT COMPLETE\n"
            << "========================================\n\n"
            << "Runs completed: "
            << rates.size() * repetitions
            << "\n"
            << "Output:\n"
            << output
            << "\n";

        return 0;
    }
    catch (const std::exception& error) {
        std::cerr
            << "\nFATAL ERROR:\n"
            << error.what()
            << "\n";
        return 1;
    }
    catch (...) {
        std::cerr
            << "\nUNKNOWN FATAL ERROR\n";
        return 1;
    }
}



