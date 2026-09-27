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

    constexpr std::size_t queue_capacity = 256;
    constexpr std::size_t max_workers = 4;
    constexpr std::uint64_t event_count = 20;

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

    pipeline.set_active_workers(2);
    pipeline.set_batch_size(1);
    pipeline.start();

    const auto unique_suffix =
        std::chrono::steady_clock::now()
            .time_since_epoch()
            .count();

    const std::string group_id =
        "nexusflow-bridge-" +
        std::to_string(unique_suffix);

    KafkaConsumerAdapter consumer(
        "localhost:19092",
        "nexusflow.bridge.test.20260913064317225",
        group_id
    );

    if (!consumer.connect()) {
        std::cerr << "KAFKA CONSUMER CONNECTION: FAIL" << std::endl;
        pipeline.stop();
        return 1;
    }
    if (!consumer.is_connected()) {
        std::cerr << "KAFKA CONSUMER CONNECTED STATE: FAIL" << std::endl;
        consumer.disconnect();
        pipeline.stop();
        return 1;
    }

    KafkaPipelineBridge bridge(
        consumer,
        queue
    );

    if (!bridge.start()) {
        std::cerr << "KAFKA PIPELINE BRIDGE START: FAIL" << std::endl;
        consumer.disconnect();
        pipeline.stop();
        return 1;
    }
    if (!bridge.is_running()) {
        std::cerr << "KAFKA PIPELINE BRIDGE RUNNING STATE: FAIL" << std::endl;
        bridge.stop();
        consumer.disconnect();
        pipeline.stop();
        return 1;
    }

    std::this_thread::sleep_for(
        std::chrono::milliseconds(2000)
    );

    std::cout
        << "Producing "
        << event_count
        << " Kafka events...\n";

    for (std::uint64_t i = 0; i < event_count; ++i) {
        const std::string key =
            std::to_string(
                900000000ULL + i
            );

        const std::string payload =
            "{\"value\":" +
            std::to_string(i) +
            "}";

        const bool produced =
            consumer.produce_test_event(
                key,
                payload
            );

        if (!produced) {
            std::cerr
                << "KAFKA PRODUCER: FAIL at event "
                << i
                << std::endl;

            bridge.stop();
            pipeline.stop();
            consumer.disconnect();
            return 1;
        }
    }

    const auto deadline =
        std::chrono::steady_clock::now() +
        std::chrono::seconds(20);

    while (
        bridge.received_count() <
            event_count &&
        std::chrono::steady_clock::now() <
            deadline
    ) {
        std::this_thread::sleep_for(
            std::chrono::milliseconds(25)
        );
    }

    const auto processing_deadline =
        std::chrono::steady_clock::now() +
        std::chrono::seconds(10);

    while (
        processed.load(
            std::memory_order_acquire
        ) < event_count &&
        std::chrono::steady_clock::now() <
            processing_deadline
    ) {
        std::this_thread::sleep_for(
            std::chrono::milliseconds(10)
        );
    }

    const auto received =
        bridge.received_count();

    const auto submitted =
        bridge.submitted_count();

    const auto rejected =
        bridge.rejected_count();

    const auto completed =
        processed.load(
            std::memory_order_acquire
        );

    bridge.stop();
    pipeline.stop();
    consumer.disconnect();

    std::cout
        << "Kafka received: "
        << received
        << "\n";

    std::cout
        << "Queue submitted: "
        << submitted
        << "\n";

    std::cout
        << "Queue rejected: "
        << rejected
        << "\n";

    std::cout
        << "Pipeline processed: "
        << completed
        << "\n";
    const bool received_ok =
        received >= event_count;

    const bool submitted_ok =
        submitted >= event_count;

    const bool rejected_ok =
        rejected == 0;

    const bool completed_ok =
        completed >= event_count;

    if (!received_ok) {
        std::cerr
            << "KAFKA RECEIVED COUNT: FAIL"
            << " expected >= "
            << event_count
            << ", actual = "
            << received
            << std::endl;
    }

    if (!submitted_ok) {
        std::cerr
            << "QUEUE SUBMITTED COUNT: FAIL"
            << " expected >= "
            << event_count
            << ", actual = "
            << submitted
            << std::endl;
    }

    if (!rejected_ok) {
        std::cerr
            << "QUEUE REJECTED COUNT: FAIL"
            << " expected = 0, actual = "
            << rejected
            << std::endl;
    }

    if (!completed_ok) {
        std::cerr
            << "PIPELINE COMPLETED COUNT: FAIL"
            << " expected >= "
            << event_count
            << ", actual = "
            << completed
            << std::endl;
    }

    if (!received_ok ||
        !submitted_ok ||
        !rejected_ok ||
        !completed_ok) {
        std::cerr
            << "KAFKA -> QUEUE -> ADAPTIVE PIPELINE: FAIL"
            << std::endl;
        return 1;
    }
std::cout << "\n";
    std::cout
        << "KAFKA -> QUEUE -> ADAPTIVE PIPELINE: PASS\n";

    return 0;
}

