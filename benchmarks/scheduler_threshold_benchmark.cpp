#include "core/scheduler/adaptive_scheduler.hpp"

#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

namespace {

using nexusflow::AdaptiveScheduler;
using nexusflow::EventPriority;
using nexusflow::ProcessingMode;
using nexusflow::SchedulerConfig;
using nexusflow::SchedulerMetrics;
using nexusflow::SchedulingDecision;

struct ExperimentCase {
    std::size_t micro_batch_size;
    std::uint64_t sla_us;
    std::size_t soft_queue_limit;
};

const char* mode_name(ProcessingMode mode) {
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

std::vector<ExperimentCase> build_cases() {
    const std::size_t batch_sizes[] = {
        8, 16, 32, 64
    };

    const std::uint64_t sla_values[] = {
        500, 1000, 2000, 5000
    };

    const std::size_t queue_limits[] = {
        16, 32, 64, 128
    };

    std::vector<ExperimentCase> cases;

    for (const auto batch_size : batch_sizes) {
        for (const auto sla_us : sla_values) {
            for (const auto queue_limit : queue_limits) {
                cases.push_back({
                    batch_size,
                    sla_us,
                    queue_limit
                });
            }
        }
    }

    return cases;
}

void write_header(std::ofstream& output) {
    output
        << "case_id,"
        << "micro_batch_size,"
        << "sla_us,"
        << "soft_queue_limit,"
        << "scenario,"
        << "queue_depth,"
        << "arrival_rate_eps,"
        << "mode,"
        << "decision_batch_size,"
        << "target_workers,"
        << "sla_bypass\n";
}

void run_case(
    std::ofstream& output,
    std::size_t case_id,
    const ExperimentCase& experiment,
    const std::string& scenario,
    std::size_t queue_depth,
    double arrival_rate_eps
) {
    SchedulerConfig config;

    config.micro_batch_size =
        experiment.micro_batch_size;

    config.soft_queue_limit =
        experiment.soft_queue_limit;

    config.hard_queue_limit =
        experiment.soft_queue_limit * 8;

    AdaptiveScheduler scheduler(config);

    SchedulerMetrics metrics;

    metrics.arrival_rate_eps =
        arrival_rate_eps;

    metrics.queue_depth =
        queue_depth;

    metrics.cpu_utilization =
        0.0;

    metrics.remaining_sla_us =
        experiment.sla_us;

    metrics.highest_priority =
        EventPriority::LOW;

    const SchedulingDecision decision =
        scheduler.decide(metrics);

    output
        << case_id << ","
        << experiment.micro_batch_size << ","
        << experiment.sla_us << ","
        << experiment.soft_queue_limit << ","
        << scenario << ","
        << queue_depth << ","
        << std::fixed
        << std::setprecision(2)
        << arrival_rate_eps << ","
        << mode_name(decision.mode) << ","
        << decision.batch_size << ","
        << decision.target_workers << ","
        << (decision.sla_bypass ? 1 : 0)
        << "\n";
}

} // namespace

int main() {
    std::cout
        << "============================================\n"
        << "NexusFlow Scheduler Threshold Experiment\n"
        << "============================================\n";

    const std::string output_path =
        "benchmarks/results/scheduler_thresholds.csv";

    std::ofstream output(output_path);

    if (!output.is_open()) {
        std::cerr
            << "ERROR: Could not open output file: "
            << output_path
            << "\n";

        return 1;
    }

    write_header(output);

    const std::vector<ExperimentCase> cases =
        build_cases();

    std::size_t case_id = 0;

    for (const auto& experiment : cases) {
        ++case_id;

        run_case(
            output,
            case_id,
            experiment,
            "LOW_LOAD",
            4,
            500.0
        );

        run_case(
            output,
            case_id,
            experiment,
            "MODERATE_LOAD",
            experiment.soft_queue_limit / 2,
            3000.0
        );

        run_case(
            output,
            case_id,
            experiment,
            "HIGH_LOAD",
            experiment.soft_queue_limit * 2,
            10000.0
        );

        run_case(
            output,
            case_id,
            experiment,
            "SLA_PRESSURE",
            experiment.soft_queue_limit,
            3000.0
        );
    }

    output.close();

    std::cout
        << "\nConfigurations tested: "
        << cases.size()
        << "\n";

    std::cout
        << "Decision scenarios per configuration: 4\n";

    std::cout
        << "Total scheduler decisions: "
        << cases.size() * 4
        << "\n";

    std::cout
        << "Results written to: "
        << output_path
        << "\n";

    std::cout
        << "\n============================================\n"
        << "Threshold experiment completed.\n"
        << "============================================\n";

    return 0;
}
