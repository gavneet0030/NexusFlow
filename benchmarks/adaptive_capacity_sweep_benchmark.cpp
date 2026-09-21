#include "core/event/event.hpp"
#include "core/processor/integrated_adaptive_pipeline.hpp"

#include <algorithm>
#include <cmath>
#include <chrono>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

using namespace nexusflow;

struct Result {
    double offered_eps;
    double throughput_eps;
    std::uint64_t p99_us;
    std::size_t workers;
    std::size_t throttled;
    std::size_t accepted;
    std::size_t processed;
};

int main()
{
    constexpr std::size_t events_per_run = 10000;
    constexpr std::size_t repetitions = 3;
    constexpr std::size_t max_workers = 16;

    const std::vector<double> loads = {
        10000.0,
        15000.0,
        20000.0,
        25000.0,
        30000.0,
        35000.0,
        40000.0,
        45000.0,
        50000.0
    };

    std::cout << "============================================\n";
    std::cout << "NEXUSFLOW ADAPTIVE CAPACITY SWEEP\n";
    std::cout << "============================================\n\n";

    std::cout
        << std::left
        << std::setw(10) << "LoadEPS"
        << std::setw(15) << "Throughput"
        << std::setw(12) << "P99us"
        << std::setw(10) << "Workers"
        << std::setw(12) << "Throttled"
        << std::setw(10) << "Integrity"
        << "\n";

    std::cout << std::string(69, '-') << "\n";

    std::vector<Result> results;

    for (double offered_eps : loads) {

        double throughput_sum = 0.0;
        double p99_sum = 0.0;
        double workers_sum = 0.0;
        double throttled_sum = 0.0;

        bool integrity = true;

        for (std::size_t rep = 0; rep < repetitions; ++rep) {

            IntegratedAdaptivePipeline pipeline(
                4096,
                max_workers);

            pipeline.start();

            const auto start =
                std::chrono::steady_clock::now();

            std::size_t accepted = 0;

            const double interval_us =
                1000000.0 / offered_eps;

            double next_event_us = 0.0;

            for (std::size_t i = 0; i < events_per_run; ++i) {

                Event event;
                event.id =
                    static_cast<std::uint64_t>(rep) *
                        events_per_run +
                    i;

                event.timestamp_ns =
                    static_cast<std::uint64_t>(
                        std::chrono::duration_cast<
                            std::chrono::nanoseconds>(
                            std::chrono::steady_clock::now()
                                .time_since_epoch())
                            .count());

                event.priority = EventPriority::NORMAL;
                event.value = static_cast<double>(i);
                event.source = "capacity_sweep";
                event.type = "synthetic";

                if (pipeline.submit(event)) {
                    ++accepted;
                }

                next_event_us += interval_us;

                if (interval_us > 0.0) {
                    const auto target =
                        start +
                        std::chrono::microseconds(
                            static_cast<std::int64_t>(
                                next_event_us));

                    std::this_thread::sleep_until(target);
                }
            }

            pipeline.drain();

            const auto end =
                std::chrono::steady_clock::now();

            const double seconds =
                std::chrono::duration<double>(
                    end - start)
                    .count();

            const double throughput =
                seconds > 0.0
                    ? static_cast<double>(accepted) / seconds
                    : 0.0;

            const auto samples =
                pipeline.latency_samples_us();

            std::uint64_t p99 = 0;

            if (!samples.empty()) {
                auto sorted = samples;

                std::sort(
                    sorted.begin(),
                    sorted.end());

                const std::size_t index =
                    std::min(
                        sorted.size() - 1,
                        static_cast<std::size_t>(
                            sorted.size() * 0.99));

                p99 = static_cast<std::uint64_t>(sorted[index]);
            }

            const auto metrics =
                pipeline.metrics();

            const std::size_t processed =
                static_cast<std::size_t>(metrics.processed);

            const std::size_t throttled =
                static_cast<std::size_t>(metrics.throttled);

            const std::size_t workers =
                pipeline.active_workers();

            if (accepted != processed ||
                metrics.submitted != events_per_run ||
                metrics.accepted != accepted ||
                metrics.processed != accepted) {
                integrity = false;
            }

            throughput_sum += throughput;
            p99_sum += static_cast<double>(p99);
            workers_sum += static_cast<double>(workers);
            throttled_sum += static_cast<double>(throttled);

        }

        Result result;

        result.offered_eps = offered_eps;
        result.throughput_eps =
            throughput_sum /
            static_cast<double>(repetitions);

        result.p99_us =
            static_cast<std::uint64_t>(
                p99_sum /
                static_cast<double>(repetitions));

        result.workers =
            static_cast<std::size_t>(
                workers_sum /
                static_cast<double>(repetitions));

        result.throttled =
            static_cast<std::size_t>(
                throttled_sum /
                static_cast<double>(repetitions));

        result.accepted = events_per_run;
        result.processed = events_per_run;

        results.push_back(result);

        std::cout
            << std::left
            << std::setw(10)
            << static_cast<std::size_t>(offered_eps)
            << std::setw(15)
            << std::fixed
            << std::setprecision(2)
            << result.throughput_eps
            << std::setw(12)
            << result.p99_us
            << std::setw(10)
            << result.workers
            << std::setw(12)
            << result.throttled
            << std::setw(10)
            << (integrity ? "PASS" : "FAIL")
            << "\n";
    }

    std::cout << "\n";
    std::cout << "============================================\n";
    std::cout << "CAPACITY SCALING VALIDATION\n";
    std::cout << "============================================\n";

    bool scaling_pass = true;

    for (const auto& result : results) {

        const double expected_workers =
            result.offered_eps / 5000.0;

        const std::size_t expected =
            static_cast<std::size_t>(
                std::ceil(expected_workers));

        const std::size_t bounded =
            std::min(
                max_workers,
                std::max<std::size_t>(1, expected));

        const std::size_t tolerance = 2;

        const bool valid =
            result.workers + tolerance >= bounded &&
            result.workers <= max_workers;

        if (!valid) {
            scaling_pass = false;
        }

        std::cout
            << "Load "
            << static_cast<std::size_t>(result.offered_eps)
            << " EPS -> expected approximately "
            << bounded
            << " workers, observed "
            << result.workers
            << " -> "
            << (valid ? "PASS" : "FAIL")
            << "\n";
    }

    std::cout << "\n";

    if (scaling_pass) {
        std::cout << "ARRIVAL CAPACITY SCALING: PASS\n";
    }
    else {
        std::cout << "ARRIVAL CAPACITY SCALING: FAIL\n";
    }

    std::cout << "\n============================================\n";

    return scaling_pass ? 0 : 1;
}
