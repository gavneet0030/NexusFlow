#include "kafka_pipeline_bridge.hpp"

#include <chrono>
#include <cstdint>
#include <regex>
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
    recovery_attempts_.store(0);
    successful_recoveries_.store(0);

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

std::uint64_t KafkaPipelineBridge::recovery_attempts() const {
    return recovery_attempts_.load(
        std::memory_order_acquire
    );
}

std::uint64_t KafkaPipelineBridge::successful_recoveries() const {
    return successful_recoveries_.load(
        std::memory_order_acquire
    );
}
void KafkaPipelineBridge::run() {
    while (running_.load()) {
        KafkaEvent kafka_event;

        if (!consumer_.poll(kafka_event, 100)) {
            if (!consumer_.is_connected()) {
                recovery_attempts_.fetch_add(
                    1,
                    std::memory_order_relaxed
                );

                if (consumer_.reconnect(3, 250)) {
                    successful_recoveries_.fetch_add(
                        1,
                        std::memory_order_relaxed
                    );
                }
            }

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

        const std::regex value_pattern(
            R"("value"\s*:\s*(-?[0-9]+(?:\.[0-9]+)?))"
        );

        const std::regex type_pattern(
            R"REGEX("type"\s*:\s*"([^"]+)")REGEX"
        );

        const std::regex priority_pattern(
            R"REGEX("priority"\s*:\s*"([^"]+)")REGEX"
        );

        std::smatch match;

        if (std::regex_search(
                kafka_event.payload,
                match,
                value_pattern)) {
            try {
                event.value = std::stod(match[1].str());
            }
            catch (...) {
                event.value = 0.0;
            }
        }

        if (std::regex_search(
                kafka_event.payload,
                match,
                type_pattern)) {
            event.type = match[1].str();
        }

        if (std::regex_search(
                kafka_event.payload,
                match,
                priority_pattern)) {

            const std::string priority =
                match[1].str();

            if (priority == "critical") {
                event.priority =
                    nexusflow::EventPriority::CRITICAL;
            }
            else if (priority == "high") {
                event.priority =
                    nexusflow::EventPriority::HIGH;
            }
            else if (priority == "low") {
                event.priority =
                    nexusflow::EventPriority::LOW;
            }
            else {
                event.priority =
                    nexusflow::EventPriority::NORMAL;
            }
        }

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



