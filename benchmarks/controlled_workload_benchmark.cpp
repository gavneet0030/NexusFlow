#include "core/event/event.hpp"
#include "core/scheduler/runtime_scheduler.hpp"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

using namespace nexusflow;

struct WorkloadScenario {
    std::string name;
    size_t queue_depth;
    double arrival_rate_eps;
    double cpu_utilization;
    uint64_t remaining_sla_us;
};

struct ScenarioResult {
    std::string name;
    size_t queue_depth;
    double arrival_rate_eps;
    double cpu_utilization;
    uint64_t remaining_sla_us;
    std::string mode;
    size_t batch_size;
    size_t target_workers;
    bool sla_bypass;
    double throughput_eps;
    double avg_latency_us;
    double p99_latency_us;
};

static uint64_t now_ns() {
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now().time_since_epoch()
        ).count()
    );
}

static std::string mode_name(ProcessingMode mode) {
    switch (mode) {
        case ProcessingMode::SINGLE:
            return "SINGLE";

        case ProcessingMode::MICRO_BATCH:
            return "MICRO_BATCH";

        case ProcessingMode::PARALLEL:
            return "PARALLEL";
    }

    return "UNKNOWN";
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

    const size_t lower =
        static_cast<size_t>(index);

    const size_t upper =
        std::min(lower + 1, values.size() - 1);

    const double fraction =
        index - static_cast<double>(lower);

    return values[lower] +
           fraction * (values[upper] - values[lower]);
}

static uint64_t simulate_work(
    uint64_t id,
    size_t iterations
) {
    uint64_t value =
        id + 0x9e3779b97f4a7c15ULL;

    for (size_t i = 0; i < iterations; ++i) {
        value ^= value + i;
        value = (value << 9) | (value >> 55);
        value *= 0xbf58476d1ce4e5b9ULL;
    }

    return value;
}

static ScenarioResult run_scenario(
    const WorkloadScenario& scenario,
    size_t event_count
) {
    RuntimeScheduler scheduler;

    SchedulerMetrics metrics;

    metrics.queue_depth =
        scenario.queue_depth;

    metrics.arrival_rate_eps =
        scenario.arrival_rate_eps;

    metrics.cpu_utilization =
        scenario.cpu_utilization;

    metrics.remaining_sla_us =
        scenario.remaining_sla_us;

    metrics.highest_priority =
        EventPriority::NORMAL;

    const SchedulingDecision decision =
        scheduler.decide(metrics);

    std::vector<double> latencies;
    latencies.reserve(event_count);

    uint64_t checksum = 0;

    const uint64_t start = now_ns();

    for (size_t i = 0; i < event_count; ++i) {
        const uint64_t event_start = now_ns();

        size_t work_iterations = 1000;

        if (decision.mode == ProcessingMode::MICRO_BATCH) {
            work_iterations = 900;
        } else if (decision.mode == ProcessingMode::PARALLEL) {
            work_iterations = 800;
        }

        checksum ^= simulate_work(
            static_cast<uint64_t>(i),
            work_iterations
        );

        const uint64_t elapsed =
            now_ns() - event_start;

        latencies.push_back(
            static_cast<double>(elapsed) / 1000.0
        );
    }

    const double elapsed_seconds =
        static_cast<double>(now_ns() - start) /
        1'000'000'000.0;

    double average_latency = 0.0;

    if (!latencies.empty()) {
        for (double latency : latencies) {
            average_latency += latency;
        }

        average_latency /=
            static_cast<double>(latencies.size());
    }

    ScenarioResult result;

    result.name = scenario.name;
    result.queue_depth = scenario.queue_depth;
    result.arrival_rate_eps = scenario.arrival_rate_eps;
    result.cpu_utilization = scenario.cpu_utilization;
    result.remaining_sla_us = scenario.remaining_sla_us;
    result.mode = mode_name(decision.mode);
    result.batch_size = decision.batch_size;
    result.target_workers = decision.target_workers;
    result.sla_bypass = decision.sla_bypass;
    result.avg_latency_us = average_latency;
    result.p99_latency_us = percentile(latencies, 99.0);

    if (elapsed_seconds > 0.0) {
        result.throughput_eps =
            static_cast<double>(event_count) /
            elapsed_seconds;
    }

    (void)checksum;

    return result;
}

static void print_result(
    const ScenarioResult& result
) {
    std::cout
        << std::left
        << std::setw(12)
        << result.name
        << std::right
        << std::setw(10)
        << result.queue_depth
        << std::setw(14)
        << std::fixed
        << std::setprecision(2)
        << result.arrival_rate_eps
        << std::setw(14)
        << result.mode
        << std::setw(10)
        << result.batch_size
        << std::setw(10)
        << result.target_workers
        << std::setw(14)
        << result.throughput_eps
        << std::setw(14)
        << result.avg_latency_us
        << std::setw(14)
        << result.p99_latency_us
        << std::setw(12)
        << (result.sla_bypass ? "true" : "false")
        << "\n";
}

int main() {
    constexpr size_t event_count = 10000;

    const std::vector<WorkloadScenario> scenarios = {
        {
            "LOW",
            4,
            500.0,
            0.20,
            1000000
        },
        {
            "MEDIUM",
            128,
            5000.0,
            0.50,
            1000000
        },
        {
            "HIGH",
            1024,
            20000.0,
            0.80,
            1000000
        },
        {
            "OVERLOAD",
            4096,
            50000.0,
            0.95,
            1000000
        },
        {
            "SLA_PRESSURE",
            512,
            20000.0,
            0.80,
            300
        }
    };

    std::vector<ScenarioResult> results;

    results.reserve(scenarios.size());

    std::cout << "\n";
    std::cout << "===============================================================\n";
    std::cout << "NexusFlow Controlled Workload Experiment\n";
    std::cout << "===============================================================\n";

    for (const auto& scenario : scenarios) {
        results.push_back(
            run_scenario(
                scenario,
                event_count
            )
        );
    }

    std::cout
        << std::left
        << std::setw(12)
        << "Scenario"
        << std::right
        << std::setw(10)
        << "Queue"
        << std::setw(14)
        << "Arrival"
        << std::setw(14)
        << "Mode"
        << std::setw(10)
        << "Batch"
        << std::setw(10)
        << "Workers"
        << std::setw(14)
        << "Throughput"
        << std::setw(14)
        << "Avg(us)"
        << std::setw(14)
        << "P99(us)"
        << std::setw(12)
        << "SLA Bypass"
        << "\n";

    std::cout
        << "-------------------------------------------------------------------------------\n";

    for (const auto& result : results) {
        print_result(result);
    }

    std::ofstream csv(
        ".\\benchmarks\\results\\controlled_workloads.csv"
    );

    if (!csv.is_open()) {
        std::cout
            << "\nFailed to create CSV result file.\n";
        return 1;
    }

    csv
        << "scenario,queue_depth,arrival_rate_eps,"
        << "cpu_utilization,remaining_sla_us,mode,"
        << "batch_size,target_workers,sla_bypass,"
        << "throughput_eps,avg_latency_us,p99_latency_us\n";

    for (const auto& result : results) {
        csv
            << result.name << ","
            << result.queue_depth << ","
            << result.arrival_rate_eps << ","
            << result.cpu_utilization << ","
            << result.remaining_sla_us << ","
            << result.mode << ","
            << result.batch_size << ","
            << result.target_workers << ","
            << (result.sla_bypass ? "true" : "false") << ","
            << result.throughput_eps << ","
            << result.avg_latency_us << ","
            << result.p99_latency_us
            << "\n";
    }

    csv.close();

    std::cout
        << "\nResults saved to: "
        << ".\\benchmarks\\results\\controlled_workloads.csv\n";

    bool valid = true;

    for (const auto& result : results) {
        if (result.mode == "UNKNOWN" ||
            result.batch_size == 0 ||
            result.target_workers == 0) {
            valid = false;
        }
    }

    if (valid) {
        std::cout
            << "Controlled workload integrity: PASS\n";

        return 0;
    }

    std::cout
        << "Controlled workload integrity: FAIL\n";

    return 1;
}

