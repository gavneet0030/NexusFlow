#pragma once

#include "core/event/end_to_end_latency.hpp"
#include "core/event/latency.hpp"
#include "core/event/event.hpp"
#include "core/processor/event_processor.hpp"
#include "core/queue/bounded_mpmc_queue.hpp"
#include "core/queue/backpressure_policy.hpp"
#include "core/workers/adaptive_worker_pool.hpp"
#include "core/scheduler/runtime_scheduler.hpp"
#include "core/scheduler/adaptive_scheduler.hpp"

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

namespace nexusflow {

struct PipelineMetrics {

    std::uint64_t submitted{0};
    std::uint64_t accepted{0};
    std::uint64_t processed{0};
    std::uint64_t rejected{0};
    std::uint64_t throttled{0};

    std::size_t queue_depth{0};
    std::size_t active_workers{0};

    double average_latency_us{0.0};
    double max_latency_us{0.0};

    std::uint64_t checksum{0};
};

class IntegratedAdaptivePipeline {
public:

    IntegratedAdaptivePipeline(
        std::size_t queue_capacity,
        std::size_t max_workers
    );

    IntegratedAdaptivePipeline(
        std::size_t queue_capacity,
        std::size_t max_workers,
        const SchedulerConfig& scheduler_config
    );

    ~IntegratedAdaptivePipeline();

    IntegratedAdaptivePipeline(
        const IntegratedAdaptivePipeline&
    ) = delete;

    IntegratedAdaptivePipeline& operator=(
        const IntegratedAdaptivePipeline&
    ) = delete;

    void start();

    void shutdown();

    bool submit(
        const Event& event
    );

    bool submit(
        Event&& event
    );

    void drain();

    void update_scheduler();

    void set_sla_budget_us(
        std::uint64_t sla_budget_us
    );

    SchedulingDecision current_decision() const;

    PipelineMetrics metrics() const;

    std::vector<double>
    latency_samples_us() const;

    EndToEndLatencyMetrics
    end_to_end_latency_metrics() const;

    std::vector<LatencyBreakdown>
    latency_breakdown_samples() const;

    std::size_t queue_depth() const;

    std::size_t active_workers() const;

    bool fixed_mode() const;

    void set_fixed_mode(
        bool enabled
    );

    void set_fixed_mode(
        const std::string& mode
    );

private:

    bool submit_internal(
        Event event
    );

    void process_event(
        const Event& event
    );

    BoundedMPMCQueue<Event> queue_;

    BackpressurePolicy backpressure_;

    RuntimeScheduler scheduler_;

    AdaptiveWorkerPool worker_pool_;

    std::atomic<bool> started_{false};

    std::atomic<bool> shutdown_{false};

    std::chrono::steady_clock::time_point
        start_time_{};

    std::atomic<std::uint64_t>
        submitted_{0};

    std::atomic<std::uint64_t>
        accepted_{0};

    std::atomic<std::uint64_t>
        rejected_{0};

    std::atomic<std::uint64_t>
        throttled_{0};

    std::atomic<std::uint64_t>
        processed_{0};

    std::atomic<std::uint64_t>
        total_latency_ns_{0};

    std::atomic<std::uint64_t>
        max_latency_ns_{0};

    std::atomic<std::uint64_t>
        checksum_{0};

    mutable std::mutex
        latency_mutex_;

    std::vector<double>
        latency_samples_;

    mutable std::mutex
        end_to_end_latency_mutex_;

    std::vector<LatencyBreakdown>
        end_to_end_latency_samples_;

    mutable std::mutex
        decision_mutex_;

    SchedulingDecision
        current_decision_{};

    bool fixed_mode_{false};

    std::size_t fixed_workers_{1};
};

} // namespace nexusflow


