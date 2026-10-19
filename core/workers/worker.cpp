#include <chrono>
#include "core/workers/worker.hpp"

namespace nexusflow {

Worker::Worker(
    BoundedMPMCQueue<Event>& queue,
    std::atomic<std::uint64_t>& processed,
    std::atomic<std::uint64_t>& total_latency_ns,
    std::atomic<std::uint64_t>& max_latency_ns,
    std::atomic<double>& checksum
)
    : queue_(queue),
      processed_(processed),
      total_latency_ns_(total_latency_ns),
      max_latency_ns_(max_latency_ns),
      checksum_(checksum) {
}

void Worker::start() {

    finished_.store(false, std::memory_order_release);

    try {
        thread_ = std::thread(&Worker::run, this);
    } catch (...) {
        finished_.store(true, std::memory_order_release);
        throw;
    }
}

void Worker::join() {

    if (thread_.joinable()) {
        thread_.join();
    }
}

void Worker::request_failure() {

    failure_requested_.store(
        true,
        std::memory_order_release
    );
}

void Worker::reset_failure() {

    failure_requested_.store(
        false,
        std::memory_order_release
    );
}

bool Worker::failure_requested() const {

    return failure_requested_.load(
        std::memory_order_acquire
    );
}

bool Worker::finished() const {

    return finished_.load(std::memory_order_acquire);
}

const std::vector<std::uint64_t>&
Worker::latencies() const {

    return latencies_;
}

void Worker::run() {

    Event event;

    while (
        !failure_requested_.load(std::memory_order_acquire)
    ) {
        if (!queue_.pop_for(event, std::chrono::milliseconds(10))) {
            if (queue_.closed()) {
                break;
            }
            continue;
        }

        const ProcessingResult result =
            processor_.process(event);

        latencies_.push_back(
            result.latency_ns
        );

        processed_.fetch_add(
            1,
            std::memory_order_relaxed
        );

        total_latency_ns_.fetch_add(
            result.latency_ns,
            std::memory_order_relaxed
        );

        std::uint64_t current_max =
            max_latency_ns_.load(
                std::memory_order_relaxed
            );

        while (
            result.latency_ns > current_max &&
            !max_latency_ns_.compare_exchange_weak(
                current_max,
                result.latency_ns,
                std::memory_order_relaxed
            )
        ) {
        }

        double current_checksum =
            checksum_.load(
                std::memory_order_relaxed
            );

        while (
            !checksum_.compare_exchange_weak(
                current_checksum,
                current_checksum + result.result,
                std::memory_order_relaxed
            )
        ) {
        }
    }

    finished_.store(true, std::memory_order_release);
}

} // namespace nexusflow
