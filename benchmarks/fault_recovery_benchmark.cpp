#include "core/event/event.hpp"
#include "core/queue/bounded_mpmc_queue.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <thread>
#include <vector>

using namespace nexusflow;

struct FaultResult {
    uint64_t submitted{0};
    uint64_t processed_before_failure{0};
    uint64_t processed_after_recovery{0};
    uint64_t processed_total{0};
    uint64_t remaining{0};

    double failure_detection_ms{0.0};
    double recovery_time_ms{0.0};
    double total_time_ms{0.0};

    bool integrity{false};
};

void process_event(const Event& event)
{
    volatile uint64_t work = 0;

    for (uint64_t i = 0; i < 500; ++i) {
        work += (event.id + i) % 997;
    }

    (void)work;
}

int main()
{
    constexpr size_t queue_capacity = 4096;
    constexpr size_t total_events = 100000;

    BoundedMPMCQueue<Event> queue(queue_capacity);

    std::atomic<uint64_t> submitted{0};
    std::atomic<uint64_t> processed{0};

    std::atomic<bool> stop_worker{false};
    std::atomic<bool> failure_triggered{false};

    std::cout
        << "============================================================\n";

    std::cout
        << "NexusFlow Fault Injection and Recovery Benchmark\n";

    std::cout
        << "============================================================\n\n";

    const auto benchmark_start =
        std::chrono::steady_clock::now();

    std::thread worker([&]() {
        Event event;

        while (
            !stop_worker.load(
                std::memory_order_acquire
            )
        ) {
            if (queue.try_pop(event)) {
                process_event(event);

                processed.fetch_add(
                    1,
                    std::memory_order_relaxed
                );
            }
            else {
                std::this_thread::yield();
            }
        }
    });

    for (size_t i = 0; i < total_events; ++i) {
        Event event;

        event.id =
            static_cast<uint64_t>(i);

        event.timestamp_ns =
            static_cast<uint64_t>(
                std::chrono::duration_cast<
                    std::chrono::nanoseconds
                >(
                    std::chrono::steady_clock::now()
                        .time_since_epoch()
                ).count()
            );

        event.priority =
            EventPriority::NORMAL;

        event.value =
            static_cast<double>(i);

        event.source =
            "fault-injection";

        event.type =
            "synthetic";

        while (!queue.try_push(std::move(event))) {
            std::this_thread::yield();

            event.id =
                static_cast<uint64_t>(i);

            event.timestamp_ns =
                static_cast<uint64_t>(
                    std::chrono::duration_cast<
                        std::chrono::nanoseconds
                    >(
                        std::chrono::steady_clock::now()
                            .time_since_epoch()
                    ).count()
                );

            event.priority =
                EventPriority::NORMAL;

            event.value =
                static_cast<double>(i);

            event.source =
                "fault-injection";

            event.type =
                "synthetic";
        }

        submitted.fetch_add(
            1,
            std::memory_order_relaxed
        );

        if (i == total_events / 4) {
            std::this_thread::yield();
        }
    }

    while (
        processed.load(
            std::memory_order_acquire
        ) < total_events / 4
    ) {
        std::this_thread::yield();
    }

    const auto failure_start =
        std::chrono::steady_clock::now();

    const uint64_t before_failure =
        processed.load(
            std::memory_order_acquire
        );

    failure_triggered.store(
        true,
        std::memory_order_release
    );

    stop_worker.store(
        true,
        std::memory_order_release
    );

    worker.join();

    const auto failure_end =
        std::chrono::steady_clock::now();

    const uint64_t queue_after_failure =
        queue.size();

    std::cout
        << "Worker failure injected.\n";

    std::cout
        << "Processed before failure: "
        << before_failure
        << "\n";

    std::cout
        << "Events remaining after failure: "
        << queue_after_failure
        << "\n";

    const auto recovery_start =
        std::chrono::steady_clock::now();

    std::thread recovery_worker([&]() {
        Event event;

        while (
            processed.load(
                std::memory_order_acquire
            ) < total_events
        ) {
            if (queue.try_pop(event)) {
                process_event(event);

                processed.fetch_add(
                    1,
                    std::memory_order_relaxed
                );
            }
            else {
                std::this_thread::yield();
            }
        }
    });

    while (
        processed.load(
            std::memory_order_acquire
        ) < total_events
    ) {
        std::this_thread::yield();
    }

    recovery_worker.join();

    const auto recovery_end =
        std::chrono::steady_clock::now();

    const double failure_detection_ms =
        std::chrono::duration<double, std::milli>(
            failure_end -
            failure_start
        ).count();

    const double recovery_time_ms =
        std::chrono::duration<double, std::milli>(
            recovery_end -
            recovery_start
        ).count();

    const double total_time_ms =
        std::chrono::duration<double, std::milli>(
            recovery_end -
            benchmark_start
        ).count();

    const uint64_t final_processed =
        processed.load(
            std::memory_order_acquire
        );

    const uint64_t final_queue_depth =
        queue.size();

    const bool integrity =
        submitted.load() == total_events &&
        final_processed == total_events &&
        final_queue_depth == 0 &&
        failure_triggered.load();

    FaultResult result;

    result.submitted =
        submitted.load();

    result.processed_before_failure =
        before_failure;

    result.processed_after_recovery =
        final_processed - before_failure;

    result.processed_total =
        final_processed;

    result.remaining =
        final_queue_depth;

    result.failure_detection_ms =
        failure_detection_ms;

    result.recovery_time_ms =
        recovery_time_ms;

    result.total_time_ms =
        total_time_ms;

    result.integrity =
        integrity;

    std::cout
        << "\n============================================================\n";

    std::cout
        << std::fixed
        << std::setprecision(3);

    std::cout
        << "Processed after recovery: "
        << result.processed_after_recovery
        << "\n";

    std::cout
        << "Final processed:          "
        << result.processed_total
        << "\n";

    std::cout
        << "Final queue depth:        "
        << result.remaining
        << "\n";

    std::cout
        << "Recovery time:            "
        << result.recovery_time_ms
        << " ms\n";

    std::cout
        << "Total benchmark time:     "
        << result.total_time_ms
        << " ms\n";

    std::cout
        << "Integrity:                "
        << (result.integrity ? "PASS" : "FAIL")
        << "\n";

    std::cout
        << "============================================================\n";

    std::ofstream csv(
        ".\\benchmarks\\results\\fault_recovery.csv"
    );

    csv
        << "submitted,processed_before_failure,"
           "processed_after_recovery,processed_total,"
           "remaining,failure_detection_ms,"
           "recovery_time_ms,total_time_ms,integrity\n";

    csv
        << result.submitted << ","
        << result.processed_before_failure << ","
        << result.processed_after_recovery << ","
        << result.processed_total << ","
        << result.remaining << ","
        << result.failure_detection_ms << ","
        << result.recovery_time_ms << ","
        << result.total_time_ms << ","
        << (result.integrity ? "PASS" : "FAIL")
        << "\n";

    csv.close();

    std::cout
        << "\nResults written to:\n"
        << "benchmarks/results/fault_recovery.csv\n";

    return result.integrity ? 0 : 1;
}
