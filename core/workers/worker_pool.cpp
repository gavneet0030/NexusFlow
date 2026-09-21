#include "core/workers/worker_pool.hpp"

#include <stdexcept>

namespace nexusflow {

WorkerPool::WorkerPool(
    std::size_t worker_count,
    std::size_t queue_capacity
)
    : queue_(queue_capacity) {

    if (worker_count == 0) {

        throw std::invalid_argument(
            "Worker count must be greater than zero."
        );
    }

    if (queue_capacity == 0) {

        throw std::invalid_argument(
            "Queue capacity must be greater than zero."
        );
    }

    workers_.reserve(worker_count);

    for (std::size_t i = 0; i < worker_count; ++i) {

        workers_.push_back(
            create_worker()
        );
    }
}

WorkerPool::~WorkerPool() {

    shutdown();
}

std::unique_ptr<Worker>
WorkerPool::create_worker() {

    return std::make_unique<Worker>(
        queue_,
        processed_,
        total_latency_ns_,
        max_latency_ns_,
        checksum_
    );
}

void WorkerPool::start() {

    if (started_) {
        return;
    }

    started_ = true;

    for (auto& worker : workers_) {

        worker->start();
    }
}

bool WorkerPool::submit(Event event) {

    if (!started_ || shutdown_) {

        return false;
    }

    return queue_.push(
        std::move(event)
    );
}

void WorkerPool::shutdown() {

    if (shutdown_) {
        return;
    }

    shutdown_ = true;

    queue_.close();

    for (auto& worker : workers_) {

        worker->join();
    }
}

bool WorkerPool::inject_worker_failure(
    std::size_t worker_id
) {

    if (
        worker_id >= workers_.size() ||
        shutdown_
    ) {
        return false;
    }

    workers_[worker_id]->request_failure();

    return true;
}

bool WorkerPool::recover_worker(
    std::size_t worker_id
) {

    if (
        worker_id >= workers_.size() ||
        shutdown_ ||
        !started_
    ) {
        return false;
    }

    auto& worker = workers_[worker_id];

    if (!worker->failure_requested()) {
        return false;
    }

    worker->join();

    worker = create_worker();

    worker->start();

    return true;
}

bool WorkerPool::worker_failure_requested(
    std::size_t worker_id
) const {

    if (worker_id >= workers_.size()) {
        return false;
    }

    return workers_[worker_id]->failure_requested();
}

std::uint64_t WorkerPool::processed() const {

    return processed_.load(
        std::memory_order_relaxed
    );
}

std::uint64_t WorkerPool::total_latency_ns() const {

    return total_latency_ns_.load(
        std::memory_order_relaxed
    );
}

std::uint64_t WorkerPool::max_latency_ns() const {

    return max_latency_ns_.load(
        std::memory_order_relaxed
    );
}

double WorkerPool::average_latency_us() const {

    const auto count = processed();

    if (count == 0) {
        return 0.0;
    }

    return static_cast<double>(
        total_latency_ns()
    )
    /
    static_cast<double>(count)
    /
    1000.0;
}

double WorkerPool::checksum() const {

    return checksum_.load(
        std::memory_order_relaxed
    );
}

std::size_t WorkerPool::queue_size() const {

    return queue_.size();
}

std::size_t WorkerPool::worker_count() const {

    return workers_.size();
}

std::vector<std::uint64_t>
WorkerPool::collect_latencies() const {

    std::vector<std::uint64_t> result;

    result.reserve(
        static_cast<std::size_t>(
            processed()
        )
    );

    for (const auto& worker : workers_) {

        const auto& worker_latencies =
            worker->latencies();

        result.insert(
            result.end(),
            worker_latencies.begin(),
            worker_latencies.end()
        );
    }

    return result;
}

} // namespace nexusflow
