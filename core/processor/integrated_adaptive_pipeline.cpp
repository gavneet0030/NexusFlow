#include "core/processor/integrated_adaptive_pipeline.hpp"
#include <vector>

#include "core/event/latency.hpp"

#include <algorithm>
#include <cmath>
#include <thread>
#include <utility>

namespace nexusflow {

namespace {

double percentile(
    const std::vector<std::uint64_t>& input,
    double probability
) {
    if (input.empty()) {
        return 0.0;
    }

    std::vector<std::uint64_t> values = input;

    std::sort(
        values.begin(),
        values.end()
    );

    const double position =
        probability *
        static_cast<double>(
            values.size() - 1
        );

    const std::size_t lower =
        static_cast<std::size_t>(position);

    const std::size_t upper =
        std::min(
            lower + 1,
            values.size() - 1
        );

    const double fraction =
        position -
        static_cast<double>(lower);

    return static_cast<double>(values[lower]) +
           fraction *
           (
               static_cast<double>(values[upper]) -
               static_cast<double>(values[lower])
           );
}

void fill_percentiles(
    const std::vector<std::uint64_t>& values,
    double& p50,
    double& p95,
    double& p99,
    double& p999,
    double& max
) {
    if (values.empty()) {
        p50 = 0.0;
        p95 = 0.0;
        p99 = 0.0;
        p999 = 0.0;
        max = 0.0;
        return;
    }

    p50 = percentile(values, 0.50);
    p95 = percentile(values, 0.95);
    p99 = percentile(values, 0.99);
    p999 = percentile(values, 0.999);

    max = static_cast<double>(
        *std::max_element(
            values.begin(),
            values.end()
        )
    );
}

}

IntegratedAdaptivePipeline::IntegratedAdaptivePipeline(
    std::size_t queue_capacity,
    std::size_t max_workers
)
    : IntegratedAdaptivePipeline(
          queue_capacity,
          max_workers,
          SchedulerConfig{}
      ) {
}

IntegratedAdaptivePipeline::IntegratedAdaptivePipeline(
    std::size_t queue_capacity,
    std::size_t max_workers,
    const SchedulerConfig& scheduler_config
)
    : queue_(queue_capacity),
      backpressure_(
          static_cast<std::size_t>(
              static_cast<double>(queue_capacity) * 0.70
          ),
          static_cast<std::size_t>(
              static_cast<double>(queue_capacity) * 0.90
          )
      ),
      scheduler_(
          max_workers,
          scheduler_config
      ),
      worker_pool_(
          queue_,
          max_workers,
          [this](const Event& event) {
              process_event(event);
          }
      ) {
}

IntegratedAdaptivePipeline::~IntegratedAdaptivePipeline() {
    shutdown();
}

void IntegratedAdaptivePipeline::start() {

    if (started_ && !shutdown_) {
        return;
    }

    shutdown_ = false;
    started_ = true;

    start_time_ =
        std::chrono::steady_clock::now();

    worker_pool_.start();

    update_scheduler();
}

bool IntegratedAdaptivePipeline::submit(
    const Event& event
) {
    return submit_internal(event);
}

bool IntegratedAdaptivePipeline::submit(
    Event&& event
) {
    return submit_internal(
        std::move(event)
    );
}

bool IntegratedAdaptivePipeline::submit_internal(
    Event event
) {
    if (shutdown_) {
        return false;
    }

    submitted_.fetch_add(
        1,
        std::memory_order_relaxed
    );

    // Event creation timestamp.
    if (event.created_at_ns == 0) {
        event.created_at_ns =
            steady_clock_ns();
    }

    const BackpressureDecision decision =
        backpressure_.evaluate(
            queue_.size(),
            queue_.capacity()
        );

    if (
        decision.action ==
        BackpressureAction::REJECT
    ) {

        rejected_.fetch_add(
            1,
            std::memory_order_relaxed
        );

        return false;
    }

    if (
        decision.action ==
        BackpressureAction::THROTTLE
    ) {

        throttled_.fetch_add(
            1,
            std::memory_order_relaxed
        );

        std::this_thread::yield();
    }

    // Record the exact timestamp immediately before queue insertion.
    event.accepted_at_ns =
        steady_clock_ns();

    if (!queue_.try_push(std::move(event))) {

        rejected_.fetch_add(
            1,
            std::memory_order_relaxed
        );

        return false;
    }

    // The local event has moved into the queue.
    // The timestamp must therefore be attached before the move.
    //
    // This path is handled by creating the timestamp before the push.
    //
    // The queue stores the timestamp as part of the Event object.

    accepted_.fetch_add(
        1,
        std::memory_order_relaxed
    );

    scheduler_.on_event_arrival();

    return true;
}

void IntegratedAdaptivePipeline::process_event(
    const Event& event
) {
    // Worker dequeue timestamp.
    event.dequeued_at_ns =
        steady_clock_ns();

    // Queue wait is measured from acceptance to worker dequeue.
    //
    // Processing starts immediately after dequeue.
    event.processing_start_ns =
        steady_clock_ns();

    volatile double value =
        event.value + 1.0;

    for (
        int iteration = 0;
        iteration < 1000;
        ++iteration
    ) {

        value =
            value * 1.000001 +
            static_cast<double>(
                iteration % 11
            );

        value *= 0.999999;
    }

    (void)value;

    event.processing_end_ns =
        steady_clock_ns();

    const std::uint64_t
        processing_latency_ns =
            static_cast<std::uint64_t>(
                std::chrono::duration_cast<
                    std::chrono::nanoseconds
                >(
                    std::chrono::steady_clock::now().time_since_epoch()
                ).count()
            );

    const std::uint64_t
        actual_processing_latency_ns =
            event.processing_end_ns -
            event.processing_start_ns;

    processed_.fetch_add(
        1,
        std::memory_order_relaxed
    );

    total_latency_ns_.fetch_add(
        actual_processing_latency_ns,
        std::memory_order_relaxed
    );

    std::uint64_t current_max =
        max_latency_ns_.load(
            std::memory_order_relaxed
        );

    while (
        actual_processing_latency_ns >
        current_max
    ) {

        if (
            max_latency_ns_.compare_exchange_weak(
                current_max,
                actual_processing_latency_ns,
                std::memory_order_relaxed
            )
        ) {
            break;
        }
    }

    const double result =
        static_cast<double>(event.id) +
        event.value;

    const std::uint64_t checksum_value =
        static_cast<std::uint64_t>(result);

    checksum_.fetch_add(
        checksum_value,
        std::memory_order_relaxed
    );

    {
        std::lock_guard<std::mutex>
            lock(latency_mutex_);

        latency_samples_.push_back(
            static_cast<double>(
                actual_processing_latency_ns
            )
        );
    }

    // Completion timestamp must be the final timestamp.
    event.completed_at_ns =
        steady_clock_ns();

    LatencyBreakdown breakdown;

    breakdown.event_created_ns =
        event.created_at_ns;

    breakdown.accepted_ns =
        event.accepted_at_ns;

    breakdown.processing_start_ns =
        event.processing_start_ns;

    breakdown.processing_end_ns =
        event.processing_end_ns;

    breakdown.completed_ns =
        event.completed_at_ns;

    {
        std::lock_guard<std::mutex>
            lock(end_to_end_latency_mutex_);

        end_to_end_latency_samples_.push_back(
            breakdown
        );
    }

    scheduler_.on_event_processed(
        actual_processing_latency_ns / 1000
    );

    (void)processing_latency_ns;
}

void IntegratedAdaptivePipeline::set_sla_budget_us(
    std::uint64_t sla_budget_us
) {
    scheduler_.set_sla_budget_us(
        sla_budget_us
    );
}

void IntegratedAdaptivePipeline::update_scheduler() {

    if (fixed_mode_) {
        return;
    }

    scheduler_.update_queue_depth(
        queue_.size()
    );

    const SchedulingDecision decision =
        scheduler_.decide();

    std::size_t workers =
        decision.target_workers;

    if (workers == 0) {
        workers = 1;
    }

    worker_pool_.set_active_workers(
        workers
    );

    std::size_t batch_size = 1;

    switch (decision.mode) {

        case ProcessingMode::SINGLE:
            batch_size = 1;
            break;

        case ProcessingMode::MICRO_BATCH:
            batch_size =
                std::max<std::size_t>(
                    1,
                    decision.batch_size
                );
            break;

        case ProcessingMode::PARALLEL:
            batch_size = 1;
            break;
    }

    worker_pool_.set_batch_size(
        batch_size
    );

    {
        std::lock_guard<std::mutex>
            lock(decision_mutex_);

        current_decision_ =
            decision;
    }
}

void IntegratedAdaptivePipeline::drain() {

    // Drain the queue without imposing a fixed 1 ms scheduler delay.
    // Scheduler updates are performed periodically while work remains.
    std::size_t scheduler_ticks = 0;

    while (
        queue_.size() > 0
    ) {

        if ((scheduler_ticks++ & 0x0F) == 0) {
            update_scheduler();
        }

        std::this_thread::yield();
    }

    const std::uint64_t expected =
        accepted_.load(
            std::memory_order_acquire
        );

    scheduler_ticks = 0;

    while (
        processed_.load(
            std::memory_order_acquire
        ) < expected
    ) {

        if ((scheduler_ticks++ & 0x0F) == 0) {
            update_scheduler();
        }

        std::this_thread::yield();
    }

    update_scheduler();
}

void IntegratedAdaptivePipeline::shutdown() {

    if (shutdown_) {
        return;
    }

    shutdown_ = true;

    worker_pool_.stop();

    queue_.close();

    started_ = false;
}

std::size_t
IntegratedAdaptivePipeline::queue_depth() const {

    return queue_.size();
}

std::size_t
IntegratedAdaptivePipeline::active_workers() const {

    return worker_pool_.active_workers();
}

SchedulingDecision
IntegratedAdaptivePipeline::current_decision() const {

    std::lock_guard<std::mutex>
        lock(decision_mutex_);

    return current_decision_;
}

PipelineMetrics
IntegratedAdaptivePipeline::metrics() const {

    PipelineMetrics result;

    result.submitted =
        submitted_.load(
            std::memory_order_relaxed
        );

    result.accepted =
        accepted_.load(
            std::memory_order_relaxed
        );

    result.processed =
        processed_.load(
            std::memory_order_relaxed
        );

    result.rejected =
        rejected_.load(
            std::memory_order_relaxed
        );

    result.throttled =
        throttled_.load(
            std::memory_order_relaxed
        );

    result.queue_depth =
        queue_.size();

    result.active_workers =
        worker_pool_.active_workers();

    const std::uint64_t total_latency_ns =
        total_latency_ns_.load(
            std::memory_order_relaxed
        );

    const std::uint64_t max_latency_ns =
        max_latency_ns_.load(
            std::memory_order_relaxed
        );

    if (result.processed > 0) {

        result.average_latency_us =
            static_cast<double>(
                total_latency_ns
            ) /
            static_cast<double>(
                result.processed
            ) /
            1000.0;
    }

    result.max_latency_us =
        static_cast<double>(
            max_latency_ns
        ) /
        1000.0;

    result.checksum =
        checksum_.load(
            std::memory_order_relaxed
        );

    return result;
}

std::vector<double>
IntegratedAdaptivePipeline::latency_samples_us() const {

    std::lock_guard<std::mutex>
        lock(latency_mutex_);

    std::vector<double> samples;

    samples.reserve(
        latency_samples_.size()
    );

    for (
        const double latency_ns :
        latency_samples_
    ) {

        samples.push_back(
            latency_ns / 1000.0
        );
    }

    return samples;
}

EndToEndLatencyMetrics
IntegratedAdaptivePipeline::end_to_end_latency_metrics() const {

    std::vector<std::uint64_t>
        queue_wait;

    std::vector<std::uint64_t>
        processing;

    std::vector<std::uint64_t>
        completion;

    std::vector<std::uint64_t>
        end_to_end;

    {
        std::lock_guard<std::mutex>
            lock(end_to_end_latency_mutex_);

        queue_wait.reserve(
            end_to_end_latency_samples_.size()
        );

        processing.reserve(
            end_to_end_latency_samples_.size()
        );

        completion.reserve(
            end_to_end_latency_samples_.size()
        );

        end_to_end.reserve(
            end_to_end_latency_samples_.size()
        );

        for (
            const auto& sample :
            end_to_end_latency_samples_
        ) {

            queue_wait.push_back(
                sample.queue_wait_us()
            );

            processing.push_back(
                sample.processing_us()
            );

            completion.push_back(
                sample.completion_us()
            );

            end_to_end.push_back(
                sample.end_to_end_us()
            );
        }
    }

    EndToEndLatencyMetrics result;

    result.samples =
        end_to_end.size();

    fill_percentiles(
        queue_wait,
        result.queue_wait_p50_us,
        result.queue_wait_p95_us,
        result.queue_wait_p99_us,
        result.queue_wait_p999_us,
        result.queue_wait_max_us
    );

    fill_percentiles(
        processing,
        result.processing_p50_us,
        result.processing_p95_us,
        result.processing_p99_us,
        result.processing_p999_us,
        result.processing_max_us
    );

    fill_percentiles(
        completion,
        result.completion_p50_us,
        result.completion_p95_us,
        result.completion_p99_us,
        result.completion_p999_us,
        result.completion_max_us
    );

    fill_percentiles(
        end_to_end,
        result.end_to_end_p50_us,
        result.end_to_end_p95_us,
        result.end_to_end_p99_us,
        result.end_to_end_p999_us,
        result.end_to_end_max_us
    );

    return result;
}

std::vector<LatencyBreakdown>
IntegratedAdaptivePipeline::latency_breakdown_samples() const {

    std::lock_guard<std::mutex>
        lock(end_to_end_latency_mutex_);

    return end_to_end_latency_samples_;
}

bool IntegratedAdaptivePipeline::fixed_mode() const {
    return fixed_mode_;
}

void IntegratedAdaptivePipeline::set_fixed_mode(
    bool enabled
) {
    fixed_mode_ = enabled;

    if (!enabled) {
        fixed_workers_ = 1;
    }
}

void IntegratedAdaptivePipeline::set_fixed_mode(
    const std::string& mode
) {

    if (mode == "FIXED_SINGLE") {

        worker_pool_.set_active_workers(1);
        worker_pool_.set_batch_size(1);

        fixed_mode_ = true;
        fixed_workers_ = 1;

        return;
    }

    if (mode == "FIXED_BATCH_32") {

        worker_pool_.set_active_workers(1);
        worker_pool_.set_batch_size(32);

        fixed_mode_ = true;
        fixed_workers_ = 1;

        return;
    }

    if (mode == "FIXED_PARALLEL_8") {

        worker_pool_.set_active_workers(8);
        worker_pool_.set_batch_size(1);

        fixed_mode_ = true;
        fixed_workers_ = 8;

        return;
    }

    fixed_mode_ = false;
    fixed_workers_ = 1;
}

} // namespace nexusflow


