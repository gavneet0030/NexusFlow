#include "core/event/event.hpp"
#include "core/queue/bounded_mpmc_queue.hpp"
#include "core/queue/backpressure_policy.hpp"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <thread>
#include <vector>

using namespace nexusflow;

struct StressMetrics {
    std::atomic<uint64_t> submitted{0};
    std::atomic<uint64_t> accepted{0};
    std::atomic<uint64_t> rejected{0};
    std::atomic<uint64_t> processed{0};
};

static uint64_t now_ns() {
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now().time_since_epoch()
        ).count()
    );
}

static Event make_event(uint64_t id) {
    Event event;

    event.id = id;
    event.timestamp_ns = now_ns();
    event.priority = EventPriority::NORMAL;
    event.value = static_cast<double>(id);
    event.source = "stress-test";
    event.type = "synthetic-event";

    return event;
}

static void process_event(const Event& event) {
    uint64_t value = event.id + 1;

    for (size_t i = 0; i < 500; ++i) {
        value ^= value + i + 0x9e3779b97f4a7c15ULL;
        value = (value << 7) | (value >> 57);
    }

    (void)value;
}

int main() {
    constexpr size_t queue_capacity = 256;
    constexpr size_t producer_count = 8;
    constexpr size_t events_per_producer = 25000;

    const size_t total_events =
        producer_count * events_per_producer;

    BoundedMPMCQueue<Event> queue(queue_capacity);

    BackpressurePolicy policy(
        queue_capacity * 70 / 100,
        queue_capacity * 90 / 100
    );

    StressMetrics metrics;

    std::atomic<bool> producers_finished{false};

    const uint64_t start = now_ns();

    std::vector<std::thread> producers;
    producers.reserve(producer_count);

    for (size_t producer = 0;
         producer < producer_count;
         ++producer) {

        producers.emplace_back(
            [&, producer]() {
                for (size_t i = 0;
                     i < events_per_producer;
                     ++i) {

                    const uint64_t id =
                        static_cast<uint64_t>(
                            producer *
                            events_per_producer +
                            i
                        );

                    metrics.submitted.fetch_add(
                        1,
                        std::memory_order_relaxed
                    );

                    const auto decision =
                        policy.evaluate(
                            queue.size(),
                            queue.capacity()
                        );

                    if (decision.action ==
                        BackpressureAction::REJECT) {

                        metrics.rejected.fetch_add(
                            1,
                            std::memory_order_relaxed
                        );

                        continue;
                    }

                    if (decision.action ==
                        BackpressureAction::THROTTLE) {

                        std::this_thread::yield();
                    }

                    if (queue.try_push(
                            make_event(id))) {

                        metrics.accepted.fetch_add(
                            1,
                            std::memory_order_relaxed
                        );
                    } else {
                        metrics.rejected.fetch_add(
                            1,
                            std::memory_order_relaxed
                        );
                    }
                }
            }
        );
    }

    std::thread consumer(
        [&]() {
            Event event;

            while (true) {
                if (queue.try_pop(event)) {
                    process_event(event);

                    metrics.processed.fetch_add(
                        1,
                        std::memory_order_relaxed
                    );

                    continue;
                }

                bool all_done = true;

                for (const auto& producer : producers) {
                    if (producer.joinable()) {
                        all_done = false;
                        break;
                    }
                }

                if (producers_finished.load(
                        std::memory_order_acquire) &&
                    queue.empty()) {
                    break;
                }

                std::this_thread::yield();
            }
        }
    );

    for (auto& producer : producers) {
        producer.join();
    }

    producers_finished.store(
        true,
        std::memory_order_release
    );

    consumer.join();

    const double elapsed_seconds =
        static_cast<double>(now_ns() - start) /
        1'000'000'000.0;

    const uint64_t submitted =
        metrics.submitted.load(
            std::memory_order_relaxed
        );

    const uint64_t accepted =
        metrics.accepted.load(
            std::memory_order_relaxed
        );

    const uint64_t rejected =
        metrics.rejected.load(
            std::memory_order_relaxed
        );

    const uint64_t processed =
        metrics.processed.load(
            std::memory_order_relaxed
        );

    const double throughput =
        elapsed_seconds > 0.0
            ? static_cast<double>(processed) /
              elapsed_seconds
            : 0.0;

    std::cout << "\n";
    std::cout << "====================================================\n";
    std::cout << "NexusFlow Multi-Producer Backpressure Stress Test\n";
    std::cout << "====================================================\n";

    std::cout
        << "Queue capacity:     "
        << queue_capacity
        << "\n";

    std::cout
        << "Producer threads:   "
        << producer_count
        << "\n";

    std::cout
        << "Events per producer:"
        << events_per_producer
        << "\n";

    std::cout
        << "Total events:       "
        << total_events
        << "\n\n";

    std::cout
        << "Submitted:          "
        << submitted
        << "\n";

    std::cout
        << "Accepted:           "
        << accepted
        << "\n";

    std::cout
        << "Rejected:           "
        << rejected
        << "\n";

    std::cout
        << "Processed:          "
        << processed
        << "\n";

    std::cout
        << "Final queue depth:  "
        << queue.size()
        << "\n";

    std::cout
        << "Elapsed seconds:    "
        << std::fixed
        << std::setprecision(4)
        << elapsed_seconds
        << "\n";

    std::cout
        << "Throughput:         "
        << std::fixed
        << std::setprecision(2)
        << throughput
        << " events/sec\n";

    const double rejection_rate =
        submitted > 0
            ? static_cast<double>(rejected) /
              static_cast<double>(submitted) *
              100.0
            : 0.0;

    std::cout
        << "Rejection rate:     "
        << std::fixed
        << std::setprecision(2)
        << rejection_rate
        << "%\n";

    std::cout
        << "====================================================\n";

    if (submitted == total_events &&
        queue.empty() &&
        processed == accepted &&
        accepted + rejected == submitted) {

        std::cout
            << "Backpressure stress integrity: PASS\n";

        return 0;
    }

    std::cout
        << "Backpressure stress integrity: FAIL\n";

    return 1;
}
