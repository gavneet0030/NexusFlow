#include "core/processor/adaptive_event_pipeline.hpp"

#include "core/processor/adaptive_execution_engine.hpp"

#include <algorithm>
#include <thread>

namespace nexusflow {

AdaptiveEventPipeline::AdaptiveEventPipeline(
    std::size_t queue_capacity,
    std::size_t max_workers
)
    : queue_(queue_capacity),
      backpressure_(
          std::max<std::size_t>(1, queue_capacity * 70 / 100),
          std::max<std::size_t>(1, queue_capacity * 90 / 100)
      ),
      scheduler_(max_workers),
      execution_engine_(
          new AdaptiveExecutionEngine()
      ) {
}

AdaptiveEventPipeline::~AdaptiveEventPipeline() {
    shutdown();

    delete execution_engine_;
    execution_engine_ = nullptr;
}

void AdaptiveEventPipeline::start() {
    if (started_) {
        return;
    }

    started_ = true;
    shutdown_ = false;
}

bool AdaptiveEventPipeline::submit(
    const Event& event
) {
    return submit_internal(event);
}

bool AdaptiveEventPipeline::submit(
    Event&& event
) {
    return submit_internal(std::move(event));
}

bool AdaptiveEventPipeline::submit_internal(
    Event event
) {
    if (shutdown_) {
        return false;
    }

    submitted_.fetch_add(
        1,
        std::memory_order_relaxed
    );

    scheduler_.on_event_arrival();

    const std::size_t depth = queue_.size();

    scheduler_.update_queue_depth(depth);

    const BackpressureDecision decision =
        backpressure_.evaluate(
            depth,
            queue_.capacity()
        );

    if (decision.action == BackpressureAction::REJECT) {
        rejected_.fetch_add(
            1,
            std::memory_order_relaxed
        );

        return false;
    }

    if (decision.action == BackpressureAction::THROTTLE) {
        throttled_.fetch_add(
            1,
            std::memory_order_relaxed
        );

        std::this_thread::yield();
    }

    if (!queue_.try_push(std::move(event))) {
        rejected_.fetch_add(
            1,
            std::memory_order_relaxed
        );

        return false;
    }

    accepted_.fetch_add(
        1,
        std::memory_order_relaxed
    );

    scheduler_.update_queue_depth(
        queue_.size()
    );

    return true;
}

std::size_t AdaptiveEventPipeline::process() {
    if (shutdown_) {
        return 0;
    }

    const SchedulingDecision decision =
        current_decision();

    std::vector<Event> batch;

    batch.reserve(decision.batch_size);

    for (std::size_t i = 0;
         i < decision.batch_size;
         ++i) {

        Event event;

        if (!queue_.try_pop(event)) {
            break;
        }

        batch.push_back(
            std::move(event)
        );
    }

    if (batch.empty()) {
        return 0;
    }

    const std::uint64_t before =
        processed_.load(
            std::memory_order_relaxed
        );

    execution_engine_->execute(
        batch,
        decision
    );

    const std::uint64_t count =
        static_cast<std::uint64_t>(
            batch.size()
        );

    processed_.fetch_add(
        count,
        std::memory_order_relaxed
    );

    scheduler_.update_queue_depth(
        queue_.size()
    );

    const std::uint64_t after =
        processed_.load(
            std::memory_order_relaxed
        );

    if (after > before) {
        scheduler_.on_event_processed(0);
    }

    return batch.size();
}

void AdaptiveEventPipeline::shutdown() {
    if (shutdown_) {
        return;
    }

    shutdown_ = true;
    queue_.close();
}

std::size_t AdaptiveEventPipeline::queue_depth() const {
    return queue_.size();
}

SchedulingDecision AdaptiveEventPipeline::current_decision() const {
    return scheduler_.decide();
}

PipelineMetrics AdaptiveEventPipeline::metrics() const {
    PipelineMetrics result;

    result.submitted =
        submitted_.load(
            std::memory_order_relaxed
        );

    result.accepted =
        accepted_.load(
            std::memory_order_relaxed
        );

    result.throttled =
        throttled_.load(
            std::memory_order_relaxed
        );

    result.rejected =
        rejected_.load(
            std::memory_order_relaxed
        );

    result.processed =
        processed_.load(
            std::memory_order_relaxed
        );

    return result;
}

} // namespace nexusflow
