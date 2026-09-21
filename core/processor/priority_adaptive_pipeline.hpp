#pragma once

#include "core/event/event.hpp"
#include "core/queue/priority_event_queue.hpp"
#include "core/scheduler/runtime_scheduler.hpp"

#include <atomic>
#include <cstddef>
#include <cstdint>

namespace nexusflow {

struct PriorityPipelineMetrics {
    uint64_t submitted{0};
    uint64_t accepted{0};
    uint64_t rejected{0};
    uint64_t processed{0};

    uint64_t critical_processed{0};
    uint64_t high_processed{0};
    uint64_t normal_processed{0};
    uint64_t low_processed{0};
};

class PriorityAdaptivePipeline {
public:
    explicit PriorityAdaptivePipeline(size_t queue_capacity = 4096);

    void start();

    bool submit(const Event& event);
    bool submit(Event&& event);

    size_t process();

    void shutdown();

    size_t queue_depth() const;

    SchedulingDecision current_decision() const;

    PriorityPipelineMetrics metrics() const;

private:
    bool submit_internal(Event event);

    void process_event(const Event& event);

    PriorityEventQueue queue_;
    RuntimeScheduler scheduler_;

    std::atomic<uint64_t> submitted_{0};
    std::atomic<uint64_t> accepted_{0};
    std::atomic<uint64_t> rejected_{0};
    std::atomic<uint64_t> processed_{0};

    std::atomic<uint64_t> critical_processed_{0};
    std::atomic<uint64_t> high_processed_{0};
    std::atomic<uint64_t> normal_processed_{0};
    std::atomic<uint64_t> low_processed_{0};

    std::atomic<bool> started_{false};
    std::atomic<bool> shutdown_{false};

    SchedulingDecision last_decision_{};
};

} // namespace nexusflow

