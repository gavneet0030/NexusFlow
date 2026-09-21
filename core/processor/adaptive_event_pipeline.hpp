#pragma once

#include "core/event/event.hpp"
#include "core/queue/backpressure_policy.hpp"
#include "core/queue/bounded_mpmc_queue.hpp"
#include "core/scheduler/runtime_scheduler.hpp"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace nexusflow {

class AdaptiveExecutionEngine;

struct PipelineMetrics {
    std::uint64_t submitted{0};
    std::uint64_t accepted{0};
    std::uint64_t throttled{0};
    std::uint64_t rejected{0};
    std::uint64_t processed{0};
};

class AdaptiveEventPipeline {
public:
    explicit AdaptiveEventPipeline(
        std::size_t queue_capacity = 4096,
        std::size_t max_workers = 16
    );

    ~AdaptiveEventPipeline();

    void start();

    bool submit(const Event& event);

    bool submit(Event&& event);

    std::size_t process();

    void shutdown();

    std::size_t queue_depth() const;

    SchedulingDecision current_decision() const;

    PipelineMetrics metrics() const;

private:
    bool submit_internal(Event event);

    BoundedMPMCQueue<Event> queue_;

    BackpressurePolicy backpressure_;

    RuntimeScheduler scheduler_;

    AdaptiveExecutionEngine* execution_engine_;

    std::atomic<std::uint64_t> submitted_{0};
    std::atomic<std::uint64_t> accepted_{0};
    std::atomic<std::uint64_t> throttled_{0};
    std::atomic<std::uint64_t> rejected_{0};
    std::atomic<std::uint64_t> processed_{0};

    bool started_{false};
    bool shutdown_{false};
};

} // namespace nexusflow
