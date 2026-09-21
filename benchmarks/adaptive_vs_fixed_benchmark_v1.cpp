#include "core/event/event.hpp"
#include "core/processor/adaptive_execution_engine.hpp"
#include "core/scheduler/adaptive_scheduler.hpp"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

using namespace nexusflow;

struct BenchmarkResult {
    std::string strategy;
    std::size_t processed{0};
    double throughput_eps{0.0};
    double p50_us{0.0};
    double p95_us{0.0};
    double p99_us{0.0};
    double p999_us{0.0};
    double max_us{0.0};
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

    std::sort(values.begin(), values.end());

    const double index =
        (percentile_value / 100.0) *
        static_cast<double>(values.size() - 1);

    const std::size_t lower =
        static_cast<std::size_t>(index);

    const std::size_t upper =
        std::min(
            lower + 1,
            values.size() - 1
        );

    const double fraction =
        index - static_cast<double>(lower);

    return values[lower] +
           fraction *
           (values[upper] - values[lower]);
}

static std::vector<Event> create_workload(
    std::size_t event_count
) {
    std::vector<Event> events;
    events.reserve(event_count);

    const std::uint64_t timestamp = now_ns();

    for (std::size_t i = 0; i < event_count; ++i) {
        Event event;

        event.id =
            static_cast<std::uint64_t>(i);

        event.timestamp_ns =
            timestamp;

        event.priority =
            EventPriority::NORMAL;

        event.value =
            static_cast<double>(i % 1000) * 0.01;

        event.source =
            "benchmark";

        event.type =
            "synthetic_event";

        events.push_back(event);
    }

    return events;
}

static BenchmarkResult build_result(
    const std::string& strategy,
    const ExecutionMetrics& metrics,
    double elapsed_seconds
) {
    BenchmarkResult result;

    result.strategy = strategy;

    result.processed =
        metrics.events_processed;

    result.checksum =
        metrics.checksum;

    if (!metrics.latencies_ns.empty()) {
        std::vector<double> latencies_us;

        latencies_us.reserve(
            metrics.latencies_ns.size()
        );

        for (const std::uint64_t latency_ns :
             metrics.latencies_ns) {
            latencies_us.push_back(
                static_cast<double>(latency_ns) /
                1000.0
            );
        }

        result.p50_us =
            percentile(latencies_us, 50.0);

        result.p95_us =
            percentile(latencies_us, 95.0);

        result.p99_us =
            percentile(latencies_us, 99.0);

        result.p999_us =
            percentile(latencies_us, 99.9);

        result.max_us =
            *std::max_element(
                latencies_us.begin(),
                latencies_us.end()
            );
    }

    if (elapsed_seconds > 0.0) {
        result.throughput_eps =
            static_cast<double>(result.processed) /
            elapsed_seconds;
    }

    return result;
}

static BenchmarkResult run_fixed_single(
    const std::vector<Event>& events
) {
    AdaptiveExecutionEngine engine(1);

    SchedulingDecision decision;

    decision.mode =
        ProcessingMode::SINGLE;

    decision.batch_size =
        1;

    decision.target_workers =
        1;

    decision.sla_bypass =
        false;

    const std::uint64_t start = now_ns();

    const ExecutionMetrics metrics =
        engine.execute(events, decision);

    const double elapsed_seconds =
        static_cast<double>(now_ns() - start) /
        1'000'000'000.0;

    return build_result(
        "FIXED_SINGLE",
        metrics,
        elapsed_seconds
    );
}

static BenchmarkResult run_fixed_batch(
    const std::vector<Event>& events
) {
    AdaptiveExecutionEngine engine(1);

    SchedulingDecision decision;

    decision.mode =
        ProcessingMode::MICRO_BATCH;

    decision.batch_size =
        32;

    decision.target_workers =
        1;

    decision.sla_bypass =
        false;

    const std::uint64_t start = now_ns();

    const ExecutionMetrics metrics =
        engine.execute(events, decision);

    const double elapsed_seconds =
        static_cast<double>(now_ns() - start) /
        1'000'000'000.0;

    return build_result(
        "FIXED_BATCH_32",
        metrics,
        elapsed_seconds
    );
}

static BenchmarkResult run_fixed_parallel(
    const std::vector<Event>& events
) {
    AdaptiveExecutionEngine engine(16);

    SchedulingDecision decision;

    decision.mode =
        ProcessingMode::PARALLEL;

    decision.batch_size =
        64;

    decision.target_workers =
        8;

    decision.sla_bypass =
        false;

    const std::uint64_t start = now_ns();

    const ExecutionMetrics metrics =
        engine.execute(events, decision);

    const double elapsed_seconds =
        static_cast<double>(now_ns() - start) /
        1'000'000'000.0;

    return build_result(
        "FIXED_PARALLEL_8",
        metrics,
        elapsed_seconds
    );
}

static BenchmarkResult run_adaptive(
    const std::vector<Event>& events
) {
    AdaptiveExecutionEngine engine(16);

    AdaptiveScheduler scheduler;

    SchedulingDecision decision;

    SchedulerMetrics scheduler_metrics;

    scheduler_metrics.arrival_rate_eps =
        20000.0;

    scheduler_metrics.queue_depth =
        events.size();

    scheduler_metrics.cpu_utilization =
        0.50;

    scheduler_metrics.remaining_sla_us =
        1000000;

    scheduler_metrics.highest_priority =
        EventPriority::NORMAL;

    decision =
        scheduler.decide(
            scheduler_metrics
        );

    std::cout
        << "Adaptive decision: "
        << "mode=";

    if (decision.mode ==
        ProcessingMode::SINGLE) {
        std::cout << "SINGLE";
    } else if (
        decision.mode ==
        ProcessingMode::MICRO_BATCH) {
        std::cout << "MICRO_BATCH";
    } else {
        std::cout << "PARALLEL";
    }

    std::cout
        << " | batch="
        << decision.batch_size
        << " | workers="
        << decision.target_workers
        << "\n\n";

    const std::uint64_t start = now_ns();

    const ExecutionMetrics metrics =
        engine.execute(events, decision);

    const double elapsed_seconds =
        static_cast<double>(now_ns() - start) /
        1'000'000'000.0;

    return build_result(
        "ADAPTIVE",
        metrics,
        elapsed_seconds
    );
}

static void print_result(
    const BenchmarkResult& result
) {
    std::cout
        << std::left
        << std::setw(20)
        << result.strategy
        << std::right
        << std::setw(14)
        << std::fixed
        << std::setprecision(2)
        << result.throughput_eps
        << std::setw(12)
        << result.p50_us
        << std::setw(12)
        << result.p95_us
        << std::setw(12)
        << result.p99_us
        << std::setw(12)
        << result.p999_us
        << std::setw(12)
        << result.max_us
        << "\n";
}

int main() {
    constexpr std::size_t event_count =
        30000;

    std::cout << "\n";
    std::cout
        << "===============================================\n";

    std::cout
        << "NexusFlow Real Adaptive vs Fixed Experiment\n";

    std::cout
        << "===============================================\n";

    std::cout
        << "Events: "
        << event_count
        << "\n\n";

    const std::vector<Event> events =
        create_workload(event_count);

    std::cout
        << "Running fixed single...\n";

    const BenchmarkResult fixed_single =
        run_fixed_single(events);

    std::cout
        << "Running fixed micro-batch...\n";

    const BenchmarkResult fixed_batch =
        run_fixed_batch(events);

    std::cout
        << "Running fixed parallel...\n";

    const BenchmarkResult fixed_parallel =
        run_fixed_parallel(events);

    std::cout
        << "Running adaptive...\n";

    const BenchmarkResult adaptive =
        run_adaptive(events);

    std::cout << "\n";

    std::cout
        << std::left
        << std::setw(20)
        << "Strategy"
        << std::right
        << std::setw(14)
        << "Throughput"
        << std::setw(12)
        << "P50(us)"
        << std::setw(12)
        << "P95(us)"
        << std::setw(12)
        << "P99(us)"
        << std::setw(12)
        << "P99.9(us)"
        << std::setw(12)
        << "Max(us)"
        << "\n";

    std::cout
        << "-------------------------------------------------------------------------------\n";

    print_result(fixed_single);
    print_result(fixed_batch);
    print_result(fixed_parallel);
    print_result(adaptive);

    std::cout
        << "-------------------------------------------------------------------------------\n";

    const double adaptive_vs_batch =
        fixed_batch.throughput_eps > 0.0
            ? (
                (
                    adaptive.throughput_eps -
                    fixed_batch.throughput_eps
                ) /
                fixed_batch.throughput_eps
            ) * 100.0
            : 0.0;

    const double adaptive_vs_single =
        fixed_single.throughput_eps > 0.0
            ? (
                (
                    adaptive.throughput_eps -
                    fixed_single.throughput_eps
                ) /
                fixed_single.throughput_eps
            ) * 100.0
            : 0.0;

    std::cout
        << "Adaptive throughput vs fixed batch: "
        << std::fixed
        << std::setprecision(2)
        << adaptive_vs_batch
        << "%\n";

    std::cout
        << "Adaptive throughput vs fixed single: "
        << adaptive_vs_single
        << "%\n";

    std::ofstream csv(
        ".\\benchmarks\\results\\adaptive_vs_fixed.csv"
    );

    if (csv.is_open()) {
        csv
            << "strategy,processed,throughput_eps,"
            << "p50_us,p95_us,p99_us,p999_us,max_us,checksum\n";

        const auto write_csv =
            [&csv](
                const BenchmarkResult& result
            ) {
                csv
                    << result.strategy << ","
                    << result.processed << ","
                    << result.throughput_eps << ","
                    << result.p50_us << ","
                    << result.p95_us << ","
                    << result.p99_us << ","
                    << result.p999_us << ","
                    << result.max_us << ","
                    << result.checksum
                    << "\n";
            };

        write_csv(fixed_single);
        write_csv(fixed_batch);
        write_csv(fixed_parallel);
        write_csv(adaptive);

        csv.close();

        std::cout
            << "Results saved to: "
            << ".\\benchmarks\\results\\adaptive_vs_fixed.csv\n";
    } else {
        std::cout
            << "Warning: could not create CSV result file.\n";
    }

    const bool integrity =
        fixed_single.processed == event_count &&
        fixed_batch.processed == event_count &&
        fixed_parallel.processed == event_count &&
        adaptive.processed == event_count;

    if (integrity) {
        std::cout
            << "Experiment integrity: PASS\n";
    } else {
        std::cout
            << "Experiment integrity: FAIL\n";
    }

    std::cout
        << "===============================================\n";

    return integrity ? 0 : 1;
}
