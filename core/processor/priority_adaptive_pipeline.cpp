#include "core/processor/priority_adaptive_pipeline.hpp"

#include <algorithm>
#include <utility>

namespace nexusflow {

PriorityAdaptivePipeline::PriorityAdaptivePipeline(size_t queue_capacity)
    : queue_(queue_capacity) {
}

void PriorityAdaptivePipeline::start() {
    shutdown_.store(false, std::memory_order_release);
    started_.store(true, std::memory_order_release);
}

bool PriorityAdaptivePipeline::submit(const Event& event) {
    return submit_internal(event);
}

bool PriorityAdaptivePipeline::submit(Event&& event) {
    return submit_internal(std::move(event));
}

bool PriorityAdaptivePipeline::submit_internal(Event event) {
    if (!started_.load(std::memory_order_acquire) ||
        shutdown_.load(std::memory_order_acquire)) {
        return false;
    }

    submitted_.fetch_add(1, std::memory_order_relaxed);

    if (queue_.try_push(std::move(event))) {
        accepted_.fetch_add(1, std::memory_order_relaxed);
        return true;
    }

    rejected_.fetch_add(1, std::memory_order_relaxed);
    return false;
}

size_t PriorityAdaptivePipeline::process() {
    if (!started_.load(std::memory_order_acquire) ||
        shutdown_.load(std::memory_order_acquire)) {
        return 0;
    }

    Event first_event;

    if (!queue_.try_pop(first_event)) {
        return 0;
    }

    SchedulerMetrics metrics;

    metrics.queue_depth = queue_.size();
    metrics.arrival_rate_eps = 0.0;
    metrics.cpu_utilization = 0.0;
    metrics.remaining_sla_us = 1000000;
    metrics.highest_priority = first_event.priority;

    last_decision_ = scheduler_.decide(metrics);

    process_event(first_event);

    size_t processed = 1;
    const size_t batch_limit =
        std::max<size_t>(1, last_decision_.batch_size);

    while (processed < batch_limit) {
        Event event;

        if (!queue_.try_pop(event)) {
            break;
        }

        process_event(event);
        ++processed;
    }

    return processed;
}

void PriorityAdaptivePipeline::process_event(const Event& event) {
    uint64_t checksum = event.id;

    for (uint64_t i = 0; i < 1000; ++i) {
        checksum ^= static_cast<uint64_t>(event.value) + i + checksum;
        checksum = (checksum << 5) | (checksum >> 59);
    }

    (void)checksum;

    processed_.fetch_add(1, std::memory_order_relaxed);

    switch (event.priority) {
        case EventPriority::CRITICAL:
            critical_processed_.fetch_add(1, std::memory_order_relaxed);
            break;

        case EventPriority::HIGH:
            high_processed_.fetch_add(1, std::memory_order_relaxed);
            break;

        case EventPriority::NORMAL:
            normal_processed_.fetch_add(1, std::memory_order_relaxed);
            break;

        case EventPriority::LOW:
            low_processed_.fetch_add(1, std::memory_order_relaxed);
            break;
    }
}

void PriorityAdaptivePipeline::shutdown() {
    shutdown_.store(true, std::memory_order_release);
    queue_.close();
    started_.store(false, std::memory_order_release);
}

size_t PriorityAdaptivePipeline::queue_depth() const {
    return queue_.size();
}

SchedulingDecision PriorityAdaptivePipeline::current_decision() const {
    return last_decision_;
}

PriorityPipelineMetrics PriorityAdaptivePipeline::metrics() const {
    PriorityPipelineMetrics result;

    result.submitted =
        submitted_.load(std::memory_order_relaxed);

    result.accepted =
        accepted_.load(std::memory_order_relaxed);

    result.rejected =
        rejected_.load(std::memory_order_relaxed);

    result.processed =
        processed_.load(std::memory_order_relaxed);

    result.critical_processed =
        critical_processed_.load(std::memory_order_relaxed);

    result.high_processed =
        high_processed_.load(std::memory_order_relaxed);

    result.normal_processed =
        normal_processed_.load(std::memory_order_relaxed);

    result.low_processed =
        low_processed_.load(std::memory_order_relaxed);

    return result;
}

} // namespace nexusflow

