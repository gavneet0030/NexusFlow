#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <vector>

#include "core/event/event.hpp"
#include "core/queue/bounded_mpmc_queue.hpp"
#include "core/workers/adaptive_worker_pool.hpp"

using namespace nexusflow;

static Event create_event(std::uint64_t id) {
    Event event;

    event.id = id;

    event.timestamp_ns =
        static_cast<std::uint64_t>(
            std::chrono::duration_cast<
                std::chrono::nanoseconds
            >(
                std::chrono::steady_clock::now()
                    .time_since_epoch()
            ).count()
        );

    event.priority = EventPriority::NORMAL;
    event.value = static_cast<double>(id % 1000);
    event.source = "worker-pool-benchmark";
    event.type = "synthetic";

    return event;
}

static void synthetic_processing(
    const Event& event
) {
    volatile double value =
        event.value + 1.0;

    for (int iteration = 0;
         iteration < 1500;
         ++iteration) {

        value =
            std::sqrt(
                value +
                static_cast<double>(
                    iteration % 17
                ) +
                1.0
            );

        value *= 1.000001;
    }

    (void)value;
}

static void run_test(
    std::size_t worker_count
) {
    constexpr std::uint64_t event_count = 100000;
    constexpr std::size_t queue_capacity = 131072;

    BoundedMPMCQueue<Event> queue(
        queue_capacity
    );

    AdaptiveWorkerPool pool(
        queue,
        16,
        synthetic_processing
    );

    pool.set_active_workers(
        worker_count
    );

    pool.start();

    const auto start =
        std::chrono::steady_clock::now();

    for (std::uint64_t id = 0;
         id < event_count;
         ++id) {

        queue.push(
            create_event(id)
        );
    }

    pool.wait_until_processed(
        event_count
    );

    const auto end =
        std::chrono::steady_clock::now();

    const double elapsed_seconds =
        std::chrono::duration<double>(
            end - start
        ).count();

    const double throughput =
        static_cast<double>(
            event_count
        ) / elapsed_seconds;

    std::cout
        << "Workers: "
        << worker_count
        << "\n";

    std::cout
        << "Processed: "
        << pool.processed_events()
        << "\n";

    std::cout
        << "Elapsed: "
        << elapsed_seconds
        << " seconds\n";

    std::cout
        << "Throughput: "
        << throughput
        << " events/sec\n";

    std::cout
        << "Idle cycles: "
        << pool.idle_cycles()
        << "\n";

    std::cout
        << "Final queue depth: "
        << queue.size()
        << "\n";

    std::cout
        << "--------------------------------------------\n";

    pool.stop();
}

int main() {
    std::cout
        << "============================================\n";

    std::cout
        << "NexusFlow Persistent Worker Scaling Benchmark\n";

    std::cout
        << "============================================\n\n";

    const std::vector<std::size_t> worker_counts{
        1,
        2,
        4,
        8,
        16
    };

    for (
        const std::size_t worker_count :
        worker_counts
    ) {
        run_test(
            worker_count
        );
    }

    std::cout
        << "\nPersistent worker pool benchmark completed.\n";

    return 0;
}
