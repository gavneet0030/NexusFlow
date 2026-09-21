#include "streaming/kafka/kafka_client.hpp"

#include <chrono>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <set>
#include <sstream>
#include <string>
#include <thread>

namespace {

std::string make_event(std::int64_t id) {

    std::ostringstream stream;

    stream
        << "{"
        << "\"event_id\":" << id << ","
        << "\"source\":\"nexusflow-cpp\","
        << "\"event_type\":\"transaction\","
        << "\"value\":" << (100.0 + static_cast<double>(id)) << ","
        << "\"priority\":\"normal\""
        << "}";

    return stream.str();
}

bool extract_event_id(
    const std::string& payload,
    std::int64_t& id
) {

    const std::string key = "\"event_id\":";
    const std::size_t position = payload.find(key);

    if (position == std::string::npos) {
        return false;
    }

    const std::size_t start =
        position + key.size();

    std::size_t end = start;

    while (
        end < payload.size() &&
        (
            payload[end] == '-' ||
            payload[end] >= '0' &&
            payload[end] <= '9'
        )
    ) {
        ++end;
    }

    if (end == start) {
        return false;
    }

    try {
        id = std::stoll(
            payload.substr(start, end - start)
        );
    }
    catch (...) {
        return false;
    }

    return true;
}

}

int main() {

    constexpr std::int64_t event_count = 100;

    const std::string brokers =
        "localhost:19092";

    const std::string topic =
        "nexusflow-events";

    const std::string group =
        "nexusflow-cpp-integration-test";

    std::cout
        << "============================================================\n"
        << "NEXUSFLOW C++ KAFKA INTEGRATION TEST\n"
        << "============================================================\n";

    try {

        std::cout << "\nCreating producer...\n";

        nexusflow::KafkaProducer producer(
            brokers,
            topic
        );

        std::cout << "PRODUCER CREATE: PASS\n";

        std::cout
            << "\nProducing "
            << event_count
            << " events...\n";

        for (
            std::int64_t id = 0;
            id < event_count;
            ++id
        ) {

            if (!producer.produce(make_event(id))) {

                std::cerr
                    << "PRODUCER: FAILED at event "
                    << id
                    << "\n";

                return 1;
            }
        }

        if (!producer.flush(10000)) {

            std::cerr
                << "PRODUCER FLUSH: FAILED\n";

            return 1;
        }

        std::cout
            << "PRODUCER DELIVERY: PASS\n";

        std::cout
            << "\nCreating consumer...\n";

        nexusflow::KafkaConsumer consumer(
            brokers,
            group,
            topic
        );

        std::cout
            << "CONSUMER CREATE: PASS\n";

        if (!consumer.subscribe()) {

            std::cerr
                << "CONSUMER SUBSCRIBE: FAILED\n";

            return 1;
        }

        std::cout
            << "CONSUMER SUBSCRIBE: PASS\n";

        std::cout
            << "\nConsuming events...\n";

        std::set<std::int64_t> received_ids;

        const auto deadline =
            std::chrono::steady_clock::now()
            + std::chrono::seconds(20);

        while (
            received_ids.size()
                < static_cast<std::size_t>(event_count) &&
            std::chrono::steady_clock::now() < deadline
        ) {

            nexusflow::KafkaMessage message;

            if (!consumer.poll(message, 500)) {
                continue;
            }

            std::int64_t id = -1;

            if (
                !extract_event_id(
                    message.payload,
                    id
                )
            ) {

                std::cerr
                    << "INVALID PAYLOAD: "
                    << message.payload
                    << "\n";

                return 1;
            }

            received_ids.insert(id);
        }

        std::cout
            << "EXPECTED EVENTS: "
            << event_count
            << "\n";

        std::cout
            << "RECEIVED UNIQUE EVENTS: "
            << received_ids.size()
            << "\n";

        if (
            received_ids.size()
            != static_cast<std::size_t>(event_count)
        ) {

            std::cerr
                << "MESSAGE COUNT INTEGRITY: FAILED\n";

            return 1;
        }

        std::cout
            << "MESSAGE COUNT INTEGRITY: PASS\n";

        for (
            std::int64_t id = 0;
            id < event_count;
            ++id
        ) {

            if (received_ids.find(id)
                == received_ids.end()) {

                std::cerr
                    << "MISSING EVENT ID: "
                    << id
                    << "\n";

                return 1;
            }
        }

        std::cout
            << "EVENT ID INTEGRITY: PASS\n";

        if (!consumer.commit()) {

            std::cerr
                << "OFFSET COMMIT: FAILED\n";

            return 1;
        }

        std::cout
            << "OFFSET COMMIT: PASS\n";

        std::cout
            << "\n============================================================\n"
            << "C++ KAFKA INTEGRATION: PASS\n"
            << "============================================================\n"
            << "Producer: PASS\n"
            << "Consumer: PASS\n"
            << "Consumer group: PASS\n"
            << "Message integrity: PASS\n"
            << "Offset commit: PASS\n"
            << "============================================================\n";

        return 0;
    }
    catch (const std::exception& error) {

        std::cerr
            << "KAFKA INTEGRATION ERROR: "
            << error.what()
            << "\n";

        return 1;
    }
}
