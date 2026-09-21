#include "streaming/kafka_consumer/kafka_consumer_adapter.hpp"
#include "streaming/kafka_consumer/kafka_pipeline_bridge.hpp"

#include "core/event/event.hpp"
#include "core/queue/bounded_mpmc_queue.hpp"
#include "core/workers/adaptive_worker_pool.hpp"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <string>
#include <thread>

int main(int argc, char** argv) {
    using namespace nexusflow;
    using namespace nexusflow::streaming;
    using clock = std::chrono::steady_clock;

    std::uint64_t event_count = 1000;
    constexpr std::size_t queue_capacity = 4096;
    constexpr std::size_t max_workers = 8;

    if (argc < 2 || argc > 3) {
        std::cerr
            << "Usage: kafka_pipeline_bridge_benchmark <topic> [event_count]\n";
        return 1;
    }

    const std::string topic = argv[1];

    if (argc == 3) {
        try {
            event_count = std::stoull(argv[2]);
        }
        catch (...) {
            std::cerr
                << "Invalid event_count\n";
            return 1;
        }
    }

    if (event_count == 0) {
        std::cerr
            << "event_count must be greater than zero\n";
        return 1;
    }

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

    const auto suffix =
        clock::now()
            .time_since_epoch()
            .count();

    KafkaConsumerAdapter consumer(
        "localhost:9092",
        topic,
        "nexusflow-benchmark-" +
            std::to_string(suffix)
    );

    if (!consumer.connect()) {
        std::cerr << "Kafka connect failed\n";
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

    std::cout
        << "Producing "
        << event_count
        << " events...\n";

    const auto produce_start = clock::now();

    for (std::uint64_t i = 0; i < event_count; ++i) {
        if (!consumer.produce_event_async(
                std::to_string(700000000ULL + i),
                "{\"value\":" +
                    std::to_string(i) +
                    "}"
            )) {
            std::cerr
                << "Producer failed at event "
                << i
                << "\n";

            bridge.stop();
            pipeline.stop();
            consumer.disconnect();
            return 1;
        }
    }

    if (!consumer.flush_producer(30000)) {
        std::cerr
            << "Producer flush failed."
            << "\n";

        bridge.stop();
        pipeline.stop();
        consumer.disconnect();
        return 1;
    }

    const auto produce_end = clock::now();

    const auto deadline =
        clock::now() +
        std::chrono::seconds(30);

    while (
        processed.load(
            std::memory_order_acquire
        ) < event_count &&
        clock::now() < deadline
    ) {
        std::this_thread::sleep_for(
            std::chrono::milliseconds(5)
        );
    }

    const auto end = clock::now();

    const double produce_seconds =
        std::chrono::duration<double>(
            produce_end - produce_start
        ).count();

    const double end_to_end_seconds =
        std::chrono::duration<double>(
            end - produce_start
        ).count();

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

    const double producer_eps =
        produce_seconds > 0.0
            ? static_cast<double>(event_count) /
                produce_seconds
            : 0.0;

    const double end_to_end_eps =
        end_to_end_seconds > 0.0
            ? static_cast<double>(completed) /
                end_to_end_seconds
            : 0.0;

    std::cout << std::fixed
              << std::setprecision(2);

    std::cout
        << "\n===== KAFKA PIPELINE BENCHMARK =====\n";

    std::cout
        << "Topic: "
        << topic
        << "\n";

    std::cout
        << "Events produced: "
        << event_count
        << "\n";

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

    std::cout
        << "Producer throughput EPS: "
        << producer_eps
        << "\n";

    std::cout
        << "End-to-end throughput EPS: "
        << end_to_end_eps
        << "\n";

    std::cout
        << "Active workers: "
        << pipeline.active_workers()
        << "\n";

    std::cout
        << "Queue capacity: "
        << queue_capacity
        << "\n";

    const bool pass =
        received == event_count &&
        submitted == event_count &&
        rejected == 0 &&
        completed == event_count;

    if (!pass) {
        std::cout
            << "KAFKA PIPELINE BENCHMARK: FAIL\n";

        return 1;
    }

    std::cout
        << "KAFKA PIPELINE BENCHMARK: PASS\n";

    return 0;
}