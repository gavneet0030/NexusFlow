#include "streaming/kafka_consumer/kafka_consumer_adapter.hpp"
#include "streaming/kafka_consumer/kafka_pipeline_bridge.hpp"

#include "core/event/event.hpp"
#include "core/queue/bounded_mpmc_queue.hpp"
#include "core/workers/adaptive_worker_pool.hpp"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <string>
#include <thread>

int main() {
    using namespace nexusflow;
    using namespace nexusflow::streaming;

    constexpr std::size_t queue_capacity = 4096;
    constexpr std::size_t max_workers = 4;
    constexpr std::uint64_t first_batch = 1000;
    constexpr std::uint64_t second_batch = 1000;

    const std::string topic = "nexusflow-recovery-20260920";

    const auto suffix =
        std::chrono::steady_clock::now()
            .time_since_epoch()
            .count();

    const std::string group_id =
        "nexusflow-recovery-" + std::to_string(suffix);

    BoundedMPMCQueue<Event> queue(queue_capacity);

    std::atomic<std::uint64_t> processed{0};

    AdaptiveWorkerPool pipeline(
        queue,
        max_workers,
        [&processed](const Event& event) {
            (void)event;
            processed.fetch_add(
                1,
                std::memory_order_relaxed
            );
        }
    );

    pipeline.set_active_workers(4);
    pipeline.set_batch_size(1);
    pipeline.start();

    KafkaConsumerAdapter consumer(
        "localhost:19092",
        topic,
        group_id
    );

    if (!consumer.connect()) {
        std::cerr << "Initial Kafka connection failed\n";
        pipeline.stop();
        return 1;
    }

    KafkaPipelineBridge bridge(
        consumer,
        queue
    );

    if (!bridge.start()) {
        std::cerr << "Bridge start failed\n";
        consumer.disconnect();
        pipeline.stop();
        return 1;
    }

    std::this_thread::sleep_for(
        std::chrono::seconds(2)
    );

    std::cout << "Producing first batch...\n";

    for (std::uint64_t i = 0; i < first_batch; ++i) {
        if (!consumer.produce_event_async(
                std::to_string(100000000ULL + i),
                "{\"value\":" + std::to_string(i) + "}"
            )) {
            std::cerr
                << "First batch producer failed at "
                << i << "\n";

            bridge.stop();
            pipeline.stop();
            consumer.disconnect();
            return 1;
        }
    }

    if (!consumer.flush_producer(30000)) {
        std::cerr << "First batch flush failed\n";
        bridge.stop();
        pipeline.stop();
        consumer.disconnect();
        return 1;
    }

    const auto first_deadline =
        std::chrono::steady_clock::now() +
        std::chrono::seconds(30);

    while (
        processed.load(std::memory_order_acquire) <
            first_batch &&
        std::chrono::steady_clock::now() <
            first_deadline
    ) {
        std::this_thread::sleep_for(
            std::chrono::milliseconds(10)
        );
    }

    if (processed.load(std::memory_order_acquire) <
        first_batch) {
        std::cerr << "First batch was not fully processed\n";
        bridge.stop();
        pipeline.stop();
        consumer.disconnect();
        return 1;
    }

    std::cout
        << "First batch processed: "
        << processed.load()
        << "\n";

    /*
     * Exercise the adapter recovery path directly.
     *
     * The bridge remains alive, but the adapter is intentionally
     * disconnected. The bridge observes the disconnected state on
     * its next poll cycle and owns the reconnect operation.
     */
    const auto failure_start =
        std::chrono::steady_clock::now();

    consumer.disconnect();

    const auto recovery_deadline =
        std::chrono::steady_clock::now() +
        std::chrono::seconds(10);

    while (
        consumer.is_connected() == false &&
        std::chrono::steady_clock::now() <
            recovery_deadline
    ) {
        std::this_thread::sleep_for(
            std::chrono::milliseconds(20)
        );
    }

    const auto recovery_end =
        std::chrono::steady_clock::now();

    const auto recovery_window_us =
        std::chrono::duration_cast<
            std::chrono::microseconds
        >(
            recovery_end - failure_start
        ).count();

    std::cout
        << "Automatic recovery window us: "
        << recovery_window_us
        << "\n";

    std::cout
        << "Consumer reconnect count: "
        << consumer.reconnect_count()
        << "\n";

    if (!consumer.is_connected()) {
        std::cerr
            << "Automatic consumer recovery was not observed\n";

        bridge.stop();
        pipeline.stop();
        consumer.disconnect();
        return 1;
    }

    std::cout << "Producing second batch...\n";

    for (std::uint64_t i = 0; i < second_batch; ++i) {
        if (!consumer.produce_event_async(
                std::to_string(200000000ULL + i),
                "{\"value\":" + std::to_string(i) + "}"
            )) {
            std::cerr
                << "Second batch producer failed at "
                << i << "\n";

            bridge.stop();
            pipeline.stop();
            consumer.disconnect();
            return 1;
        }
    }

    if (!consumer.flush_producer(30000)) {
        std::cerr << "Second batch flush failed\n";
        bridge.stop();
        pipeline.stop();
        consumer.disconnect();
        return 1;
    }

    const auto final_target =
        first_batch + second_batch;

    const auto final_deadline =
        std::chrono::steady_clock::now() +
        std::chrono::seconds(30);

    while (
        processed.load(std::memory_order_acquire) <
            final_target &&
        std::chrono::steady_clock::now() <
            final_deadline
    ) {
        std::this_thread::sleep_for(
            std::chrono::milliseconds(10)
        );
    }

    const auto final_processed =
        processed.load(std::memory_order_acquire);

    const auto recovery_attempts =
        bridge.recovery_attempts();

    const auto successful_recoveries =
        bridge.successful_recoveries();

    const bool pass =
        final_processed >= final_target &&
        consumer.is_connected() &&
        recovery_attempts > 0 &&
        successful_recoveries > 0;

    bridge.stop();
    pipeline.stop();
    consumer.disconnect();

    std::cout
        << "\n===== KAFKA CONSUMER RECOVERY TEST =====\n";

    std::cout
        << "First batch: "
        << first_batch
        << "\n";

    std::cout
        << "Second batch: "
        << second_batch
        << "\n";

    std::cout
        << "Final processed: "
        << final_processed
        << "\n";

    std::cout
        << "Recovery attempts: "
        << recovery_attempts
        << "\n";

    std::cout
        << "Successful recoveries: "
        << successful_recoveries
        << "\n";

    std::cout
        << "Automatic recovery window us: "
        << recovery_window_us
        << "\n";

    std::cout
        << "KAFKA CONSUMER RECOVERY: "
        << (pass ? "PASS" : "FAIL")
        << "\n";

    return pass ? 0 : 1;
}
