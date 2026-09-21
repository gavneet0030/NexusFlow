#include "core/event/event.hpp"
#include "core/processor/adaptive_execution_engine.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <vector>

using namespace nexusflow;

struct ScalingResult {
    std::size_t workers{0};
    std::size_t events{0};

    double throughput_eps{0.0};

    double average_latency_us{0.0};
    double p50_us{0.0};
    double p95_us{0.0};
    double p99_us{0.0};
    double p999_us{0.0};
    double max_latency_us{0.0};

    double checksum{0.0};
};

static std::uint64_t now_ns() {
    return static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now().time_since_epoch()
        ).count()
    );
}

static double percentile(
    std::vector<double> values,
    double percentile_value
) {
    if (values.empty()) {
        return 0.0;
    }

    std::sort(
        values.begin(),
        values.end()
    );

    const double index =
        (percentile_value / 100.0) *
        static_cast<double>(
            values.size() - 1
        );

    const std::size_t lower =
        static_cast<std::size_t>(index);

    const std::size_t upper =
        std::min(
            lower + 1,
            values.size() - 1
        );

    const double fraction =
        index -
        static_cast<double>(lower);

    return values[lower] +
           fraction *
           (
               values[upper] -
               values[lower]
           );
}

static std::vector<Event> create_workload(
    std::size_t event_count
) {
    std::vector<Event> events;

    events.reserve(event_count);

    const std::uint64_t release_time =
        now_ns();

    for (std::size_t i = 0;
         i < event_count;
         ++i) {

        Event event;

        event.id =
            static_cast<std::uint64_t>(
                i + 1
            );

        event.timestamp_ns =
            release_time;

        event.priority =
            EventPriority::NORMAL;

        event.value =
            static_cast<double>(
                (i % 1000) + 1
            );

        event.source =
            "worker_scaling_benchmark";

        event.type =
            "synthetic_event";

        events.push_back(
            event
        );
    }

    return events;
}

static ScalingResult run_worker_configuration(
    const std::vector<Event>& events,
    std::size_t workers
) {
    AdaptiveExecutionEngine engine(
        workers
    );

    SchedulingDecision decision;

    decision.mode =
        ProcessingMode::PARALLEL;

    decision.batch_size =
        64;

    decision.target_workers =
        workers;

    decision.sla_bypass =
        false;

    const std::uint64_t start =
        now_ns();

    const ExecutionMetrics metrics =
        engine.execute(
            events,
            decision
        );

    const double elapsed_seconds =
        static_cast<double>(
            now_ns() - start
        ) /
        1'000'000'000.0;

    ScalingResult result;

    result.workers =
        workers;

    result.events =
        metrics.events_processed;

    result.checksum =
        metrics.checksum;

    if (elapsed_seconds > 0.0) {
        result.throughput_eps =
            static_cast<double>(
                result.events
            ) /
            elapsed_seconds;
    }

    if (!metrics.latencies_ns.empty()) {
        std::vector<double> latencies_us;

        latencies_us.reserve(
            metrics.latencies_ns.size()
        );

        for (
            const std::uint64_t latency_ns :
            metrics.latencies_ns
        ) {
            latencies_us.push_back(
                static_cast<double>(
                    latency_ns
                ) /
                1000.0
            );
        }

        double total_latency_us =
            0.0;

        for (
            const double latency_us :
            latencies_us
        ) {
            total_latency_us +=
                latency_us;
        }

        result.average_latency_us =
            total_latency_us /
            static_cast<double>(
                latencies_us.size()
            );

        result.p50_us =
            percentile(
                latencies_us,
                50.0
            );

        result.p95_us =
            percentile(
                latencies_us,
                95.0
            );

        result.p99_us =
            percentile(
                latencies_us,
                99.0
            );

        result.p999_us =
            percentile(
                latencies_us,
                99.9
            );

        result.max_latency_us =
            *std::max_element(
                latencies_us.begin(),
                latencies_us.end()
            );
    }

    return result;
}

static void print_result(
    const ScalingResult& result
) {
    std::cout
        << std::left
        << std::setw(10)
        << result.workers

        << std::right
        << std::setw(16)
        << std::fixed
        << std::setprecision(2)
        << result.throughput_eps

        << std::setw(16)
        << result.average_latency_us

        << std::setw(14)
        << result.p50_us

        << std::setw(14)
        << result.p95_us

        << std::setw(14)
        << result.p99_us

        << std::setw(14)
        << result.p999_us

        << std::setw(16)
        << result.max_latency_us

        << "\n";
}

int main() {
    constexpr std::size_t event_count =
        100000;

    const std::vector<std::size_t>
        worker_counts = {
            1,
            2,
            4,
            8,
            16
        };

    std::cout << "\n";

    std::cout
        << "===============================================\n";

    std::cout
        << "NexusFlow Worker Scaling Experiment\n";

    std::cout
        << "===============================================\n";

    std::cout
        << "Events: "
        << event_count
        << "\n";

    std::cout
        << "Mode: PARALLEL\n";

    std::cout
        << "Batch size: 64\n\n";

    std::cout
        << "Preparing workload...\n";

    const std::vector<Event> events =
        create_workload(
            event_count
        );

    std::cout
        << "Workload prepared.\n\n";

    std::cout
        << std::left
        << std::setw(10)
        << "Workers"

        << std::right
        << std::setw(16)
        << "Throughput"

        << std::setw(16)
        << "Avg(us)"

        << std::setw(14)
        << "P50(us)"

        << std::setw(14)
        << "P95(us)"

        << std::setw(14)
        << "P99(us)"

        << std::setw(14)
        << "P99.9(us)"

        << std::setw(16)
        << "Max(us)"

        << "\n";

    std::cout
        << "--------------------------------------------------------------------------------\n";

    std::vector<ScalingResult>
        results;

    results.reserve(
        worker_counts.size()
    );

    for (
        const std::size_t workers :
        worker_counts
    ) {
        std::cout
            << "Running "
            << workers
            << " worker configuration...\n";

        const ScalingResult result =
            run_worker_configuration(
                events,
                workers
            );

        results.push_back(
            result
        );
    }

    std::cout << "\n";

    for (
        const ScalingResult& result :
        results
    ) {
        print_result(
            result
        );
    }

    std::cout
        << "--------------------------------------------------------------------------------\n";

    if (!results.empty()) {
        const ScalingResult& baseline =
            results.front();

        const ScalingResult& best =
            *std::max_element(
                results.begin(),
                results.end(),
                [](
                    const ScalingResult& a,
                    const ScalingResult& b
                ) {
                    return
                        a.throughput_eps <
                        b.throughput_eps;
                }
            );

        const double throughput_improvement =
            baseline.throughput_eps > 0.0
                ? (
                    (
                        best.throughput_eps -
                        baseline.throughput_eps
                    ) /
                    baseline.throughput_eps
                ) * 100.0
                : 0.0;

        std::cout
            << "Best throughput: "
            << best.workers
            << " workers | "
            << std::fixed
            << std::setprecision(2)
            << best.throughput_eps
            << " events/sec\n";

        std::cout
            << "Throughput improvement vs 1 worker: "
            << throughput_improvement
            << "%\n";

        std::cout
            << "Baseline checksum: "
            << std::setprecision(10)
            << baseline.checksum
            << "\n";

        bool integrity_pass =
            true;

        for (
            const ScalingResult& result :
            results
        ) {
            if (
                result.events !=
                event_count
            ) {
                integrity_pass =
                    false;
            }

            const double checksum_difference =
                std::abs(
                    result.checksum -
                    baseline.checksum
                );

            const double checksum_tolerance =
                0.000001 *
                std::max(
                    1.0,
                    std::abs(
                        baseline.checksum
                    )
                );

            if (
                checksum_difference >
                checksum_tolerance
            ) {
                integrity_pass =
                    false;
            }
        }

        std::cout
            << "Experiment integrity: "
            << (
                integrity_pass
                    ? "PASS"
                    : "FAIL"
            )
            << "\n";
    }

    std::cout
        << "===============================================\n";

    return 0;
}
