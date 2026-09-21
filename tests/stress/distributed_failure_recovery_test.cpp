#include "core/event/event.hpp"
#include "core/workers/worker_pool.hpp"

#include <chrono>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <thread>

namespace {

nexusflow::Event make_event(
    std::uint64_t id
) {

    nexusflow::Event event;

    event.id = id;
    event.timestamp_ns =
        static_cast<std::uint64_t>(
            std::chrono::steady_clock::now()
                .time_since_epoch()
                .count()
        );

    event.value =
        static_cast<double>(
            (id % 1000) + 1
        );

    event.source = "failure_test";
    event.type = "synthetic";

    return event;
}

bool wait_until_processed(
    nexusflow::WorkerPool& pool,
    std::uint64_t target,
    std::uint64_t timeout_ms
) {

    const auto deadline =
        std::chrono::steady_clock::now() +
        std::chrono::milliseconds(timeout_ms);

    while (
        pool.processed() < target &&
        std::chrono::steady_clock::now() < deadline
    ) {

        std::this_thread::sleep_for(
            std::chrono::milliseconds(2)
        );
    }

    return pool.processed() >= target;
}

}

int main() {

    constexpr std::size_t worker_count = 8;
    constexpr std::size_t queue_capacity = 200000;
    constexpr std::uint64_t event_count = 100000;

    std::cout
        << "============================================================\n"
        << "NEXUSFLOW DISTRIBUTED FAILURE RECOVERY TEST\n"
        << "============================================================\n";

    nexusflow::WorkerPool pool(
        worker_count,
        queue_capacity
    );

    pool.start();

    for (
        std::uint64_t id = 0;
        id < event_count;
        ++id
    ) {

        if (!pool.submit(make_event(id))) {

            std::cerr
                << "SUBMISSION FAILED\n";

            pool.shutdown();

            return 1;
        }
    }

    const std::uint64_t failure_target = 25000;

    if (!wait_until_processed(
        pool,
        failure_target,
        5000
    )) {

        std::cerr
            << "FAILURE INJECTION TARGET NOT REACHED\n";

        pool.shutdown();

        return 1;
    }

    const auto failure_start =
        std::chrono::steady_clock::now();

    constexpr std::size_t failed_worker = 3;

    const bool failure_requested =
        pool.inject_worker_failure(
            failed_worker
        );

    if (!failure_requested) {

        std::cerr
            << "FAILURE INJECTION: FAIL\n";

        pool.shutdown();

        return 1;
    }

    const bool failure_observed =
        wait_until_processed(
            pool,
            pool.processed() + 1,
            5000
        );

    const auto recovery_start =
        std::chrono::steady_clock::now();

    const bool recovery =
        pool.recover_worker(
            failed_worker
        );

    const auto recovery_end =
        std::chrono::steady_clock::now();

    const auto recovery_us =
        std::chrono::duration_cast<
            std::chrono::microseconds
        >(
            recovery_end - recovery_start
        ).count();

    const bool completed =
        wait_until_processed(
            pool,
            event_count,
            10000
        );

    const auto total_processed =
        pool.processed();

    const double checksum =
        pool.checksum();

    pool.shutdown();

    const auto failure_to_recovery_us =
        std::chrono::duration_cast<
            std::chrono::microseconds
        >(
            recovery_end - failure_start
        ).count();

    const bool processed_integrity =
        total_processed == event_count;

    const bool checksum_valid =
        std::isfinite(checksum);

    std::cout
        << "\nFAILURE INJECTION: "
        << (failure_requested ? "PASS" : "FAIL")
        << "\n";

    std::cout
        << "FAILURE OBSERVED: "
        << (failure_observed ? "PASS" : "FAIL")
        << "\n";

    std::cout
        << "WORKER RECOVERY: "
        << (recovery ? "PASS" : "FAIL")
        << "\n";

    std::cout
        << "RECOVERY JOIN TIME (us): "
        << recovery_us
        << "\n";

    std::cout
        << "FAILURE TO RECOVERY (us): "
        << failure_to_recovery_us
        << "\n";

    std::cout
        << "TOTAL PROCESSED: "
        << total_processed
        << "\n";

    std::cout
        << "EXPECTED PROCESSED: "
        << event_count
        << "\n";

    std::cout
        << "PROCESSED INTEGRITY: "
        << (processed_integrity ? "PASS" : "FAIL")
        << "\n";

    std::cout
        << "CHECKSUM: "
        << checksum
        << "\n";

    std::cout
        << "CHECKSUM VALIDITY: "
        << (checksum_valid ? "PASS" : "FAIL")
        << "\n";

    std::cout
        << "QUEUE SIZE AFTER RECOVERY: "
        << pool.queue_size()
        << "\n";

    std::cout
        << "FINAL COMPLETION: "
        << (completed ? "PASS" : "FAIL")
        << "\n";

    const bool final_pass =
        failure_requested &&
        failure_observed &&
        recovery &&
        completed &&
        processed_integrity &&
        checksum_valid;

    std::cout
        << "\n============================================================\n"
        << "DISTRIBUTED FAILURE RECOVERY: "
        << (final_pass ? "PASS" : "FAIL")
        << "\n"
        << "============================================================\n";

    return final_pass ? 0 : 1;
}
