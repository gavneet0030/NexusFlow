#include <algorithm>
#include "streaming/event_replay/event_replay.hpp"
using nexusflow::streaming::EventReplay;
#include "core/processor/integrated_adaptive_pipeline.hpp"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string>
#include <thread>

namespace {

std::uint64_t parse_uint64(
    const std::string& value
) {
    return std::stoull(value);
}

void print_usage() {
    std::cout
        << "Usage: real_dataset_replay_benchmark.exe "
        << "[--rate EPS] "
        << "[--events COUNT] "
        << "[--input PATH]\n";
}

}

int main(int argc, char* argv[]) {

    std::string input_path =
        "data/processed/nexusflow_events.csv";

    std::uint64_t target_rate_eps = 1000;
    std::size_t event_limit = 10000;

    for (int i = 1; i < argc; ++i) {

        const std::string argument = argv[i];

        if (argument == "--rate") {
            if (i + 1 >= argc) {
                print_usage();
                return 1;
            }

            target_rate_eps =
                parse_uint64(argv[++i]);

            continue;
        }

        if (argument == "--events") {
            if (i + 1 >= argc) {
                print_usage();
                return 1;
            }

            event_limit =
                static_cast<std::size_t>(
                    parse_uint64(argv[++i])
                );

            continue;
        }

        if (argument == "--input") {
            if (i + 1 >= argc) {
                print_usage();
                return 1;
            }

            input_path = argv[++i];
            continue;
        }

        if (argument == "--help") {
            print_usage();
            return 0;
        }

        std::cout
            << "Unknown argument: "
            << argument
            << "\n";

        return 1;
    }

    if (target_rate_eps == 0) {
        std::cout
            << "Target rate must be greater than zero.\n";
        return 1;
    }

    nexusflow::streaming::EventReplay replay;

    if (!replay.load_csv(input_path)) {
        std::cout
            << "Failed to load dataset: "
            << input_path
            << "\n";
        return 1;
    }

    const std::size_t events_to_replay =
        std::min(
            event_limit,
            replay.size()
        );

    if (events_to_replay == 0) {
        std::cout
            << "No events available for replay.\n";
        return 1;
    }

    /*
     * The pipeline capacity is deliberately larger
     * than the validation workload.
     */
    nexusflow::IntegratedAdaptivePipeline pipeline(
        4096,
        8
    );

    pipeline.start();

    std::cout << "\n";
    std::cout
        << "NexusFlow Real Dataset Pipeline Replay\n";
    std::cout
        << "========================================\n";
    std::cout
        << "Input: "
        << input_path
        << "\n";
    std::cout
        << "Dataset events: "
        << replay.size()
        << "\n";
    std::cout
        << "Events to replay: "
        << events_to_replay
        << "\n";
    std::cout
        << "Target rate: "
        << target_rate_eps
        << " events/sec\n";
    std::cout << "\n";

    const auto start =
        std::chrono::steady_clock::now();

    const auto interval =
        std::chrono::nanoseconds(
            static_cast<long long>(
                1000000000ULL /
                target_rate_eps
            )
        );

    auto next_event_time = start;

    std::size_t accepted = 0;
    std::size_t rejected = 0;

    for (std::size_t i = 0;
         i < events_to_replay;
         ++i) {

        next_event_time += interval;

        while (
            std::chrono::steady_clock::now()
            < next_event_time
        ) {
            std::this_thread::yield();
        }

        nexusflow::Event event =
            replay.make_event(i);

        if (pipeline.submit(std::move(event))) {
            ++accepted;
        }
        else {
            ++rejected;
        }

        /*
         * Periodically update the adaptive scheduler
         * using the current queue state.
         */
        if ((i + 1) % 100 == 0) {
            pipeline.update_scheduler();
        }
    }

    /*
     * Allow all accepted events to finish.
     */
    pipeline.drain();

    const auto end =
        std::chrono::steady_clock::now();

    const double elapsed_seconds =
        std::chrono::duration<double>(
            end - start
        ).count();

    const auto metrics =
        pipeline.metrics();

    pipeline.shutdown();

    const double throughput =
        elapsed_seconds > 0.0
            ? static_cast<double>(
                metrics.processed
              ) / elapsed_seconds
            : 0.0;

    std::cout << "\n";
    std::cout
        << "Pipeline Replay Results\n";
    std::cout
        << "-----------------------\n";

    std::cout
        << "Submitted: "
        << metrics.submitted
        << "\n";

    std::cout
        << "Accepted: "
        << metrics.accepted
        << "\n";

    std::cout
        << "Rejected: "
        << metrics.rejected
        << "\n";

    std::cout
        << "Processed: "
        << metrics.processed
        << "\n";

    std::cout
        << "Throttled: "
        << metrics.throttled
        << "\n";

    std::cout
        << "Elapsed seconds: "
        << elapsed_seconds
        << "\n";

    std::cout
        << "Processing throughput EPS: "
        << throughput
        << "\n";

    std::cout
        << "Average latency us: "
        << metrics.average_latency_us
        << "\n";

    std::cout
        << "Max latency us: "
        << metrics.max_latency_us
        << "\n";

    std::cout
        << "Final queue depth: "
        << metrics.queue_depth
        << "\n";

    std::cout
        << "Active workers: "
        << metrics.active_workers
        << "\n";

    std::cout
        << "Checksum: "
        << metrics.checksum
        << "\n";

    std::cout << "\n";

    if (
        metrics.processed ==
        metrics.accepted
    ) {
        std::cout
            << "Integrity: PASS\n";
    }
    else {
        std::cout
            << "Integrity: FAIL\n";
        return 1;
    }

    if (rejected > 0) {
        std::cout
            << "Replay rejected events: "
            << rejected
            << "\n";
    }

    std::cout << "\n";
    std::cout
        << "REAL DATASET PIPELINE REPLAY: PASS\n";

    return 0;
}

