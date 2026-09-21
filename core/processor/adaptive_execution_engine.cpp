#include "core/processor/adaptive_execution_engine.hpp"

#include <algorithm>
#include <future>

namespace nexusflow {

AdaptiveExecutionEngine::AdaptiveExecutionEngine(
    std::size_t max_workers
)
    : max_workers_(
        std::max<std::size_t>(
            1,
            max_workers
        )
    ) {
}

ExecutionMetrics AdaptiveExecutionEngine::execute(
    const std::vector<Event>& events,
    const SchedulingDecision& decision
) {
    ExecutionMetrics metrics;

    if (events.empty()) {
        return metrics;
    }

    metrics.latencies_ns.reserve(
        events.size()
    );

    // SINGLE mode: process one event at a time.
    if (decision.mode == ProcessingMode::SINGLE) {

        for (const Event& event : events) {

            const BatchProcessingResult result =
                processor_.process_single(event);

            metrics.events_processed +=
                result.events_processed;

            metrics.total_latency_ns +=
                result.total_latency_ns;

            metrics.max_latency_ns =
                std::max(
                    metrics.max_latency_ns,
                    result.max_latency_ns
                );

            metrics.checksum +=
                result.checksum;

            metrics.latencies_ns.insert(
                metrics.latencies_ns.end(),
                result.latencies_ns.begin(),
                result.latencies_ns.end()
            );
        }

        return metrics;
    }

    // MICRO_BATCH mode: process fixed-size batches sequentially.
    if (decision.mode == ProcessingMode::MICRO_BATCH) {

        const std::size_t batch_size =
            std::max<std::size_t>(
                1,
                decision.batch_size
            );

        for (
            std::size_t start = 0;
            start < events.size();
            start += batch_size
        ) {

            const std::size_t end =
                std::min(
                    start + batch_size,
                    events.size()
                );

            std::vector<Event> batch(
                events.begin() +
                    static_cast<std::ptrdiff_t>(start),

                events.begin() +
                    static_cast<std::ptrdiff_t>(end)
            );

            const BatchProcessingResult result =
                processor_.process_batch(batch);

            metrics.events_processed +=
                result.events_processed;

            metrics.total_latency_ns +=
                result.total_latency_ns;

            metrics.max_latency_ns =
                std::max(
                    metrics.max_latency_ns,
                    result.max_latency_ns
                );

            metrics.checksum +=
                result.checksum;

            metrics.latencies_ns.insert(
                metrics.latencies_ns.end(),
                result.latencies_ns.begin(),
                result.latencies_ns.end()
            );
        }

        return metrics;
    }

    // PARALLEL mode: split work across multiple asynchronous workers.
    const std::size_t workers =
        std::clamp<std::size_t>(
            decision.target_workers,
            1,
            std::min(
                max_workers_,
                events.size()
            )
        );

    const std::size_t chunk_size =
        (
            events.size() +
            workers -
            1
        ) / workers;

    std::vector<
        std::future<BatchProcessingResult>
    > futures;

    futures.reserve(workers);

    for (
        std::size_t worker = 0;
        worker < workers;
        ++worker
    ) {

        const std::size_t start =
            worker * chunk_size;

        if (start >= events.size()) {
            break;
        }

        const std::size_t end =
            std::min(
                start + chunk_size,
                events.size()
            );

        futures.push_back(
            std::async(
                std::launch::async,
                [this, &events, start, end]() {

                    std::vector<Event> batch(
                        events.begin() +
                            static_cast<std::ptrdiff_t>(
                                start
                            ),

                        events.begin() +
                            static_cast<std::ptrdiff_t>(
                                end
                            )
                    );

                    return processor_.process_batch(
                        batch
                    );
                }
            )
        );
    }

    for (auto& future : futures) {

        const BatchProcessingResult result =
            future.get();

        metrics.events_processed +=
            result.events_processed;

        metrics.total_latency_ns +=
            result.total_latency_ns;

        metrics.max_latency_ns =
            std::max(
                metrics.max_latency_ns,
                result.max_latency_ns
            );

        metrics.checksum +=
            result.checksum;

        metrics.latencies_ns.insert(
            metrics.latencies_ns.end(),
            result.latencies_ns.begin(),
            result.latencies_ns.end()
        );
    }

    return metrics;
}

} // namespace nexusflow
