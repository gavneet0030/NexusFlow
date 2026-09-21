#include "kafka_pipeline_bridge.hpp"

#include <chrono>
#include <cstdint>
#include <string>
#include <utility>

namespace nexusflow::streaming {

KafkaPipelineBridge::KafkaPipelineBridge(
    KafkaConsumerAdapter& consumer,
    nexusflow::BoundedMPMCQueue<nexusflow::Event>& queue
)
    : consumer_(consumer),
      queue_(queue) {}

KafkaPipelineBridge::~KafkaPipelineBridge() {
    stop();
}

bool KafkaPipelineBridge::start() {
    if (running_.exchange(true)) {
        return false;
    }

    received_.store(0);
    submitted_.store(0);
    rejected_.store(0);

    thread_ = std::thread(
        &KafkaPipelineBridge::run,
        this
    );

    return true;
}

void KafkaPipelineBridge::stop() {
    if (!running_.exchange(false)) {
        return;
    }

    if (thread_.joinable()) {
        thread_.join();
    }
}

bool KafkaPipelineBridge::is_running() const {
    return running_.load();
}

std::uint64_t KafkaPipelineBridge::received_count() const {
    return received_.load();
}

std::uint64_t KafkaPipelineBridge::submitted_count() const {
    return submitted_.load();
}

std::uint64_t KafkaPipelineBridge::rejected_count() const {
    return rejected_.load();
}

void KafkaPipelineBridge::run() {
    while (running_.load()) {
        KafkaEvent kafka_event;

        if (!consumer_.poll(kafka_event, 100)) {
            continue;
        }

        received_.fetch_add(1);

        nexusflow::Event event;

        try {
            event.id = static_cast<std::uint64_t>(
                std::stoull(kafka_event.key)
            );
        }
        catch (...) {
            event.id = received_.load();
        }

        event.timestamp_ns =
            kafka_event.timestamp_ms > 0
                ? static_cast<std::uint64_t>(
                    kafka_event.timestamp_ms
                ) * 1000000ULL
                : 0ULL;

        event.priority =
            nexusflow::EventPriority::NORMAL;

        event.value = 0.0;
        event.source = "kafka";
        event.type = "stream_event";

        event.created_at_ns =
            event.timestamp_ns;

        if (queue_.try_push(std::move(event))) {
            submitted_.fetch_add(1);
        }
        else {
            rejected_.fetch_add(1);
        }
    }
}

}