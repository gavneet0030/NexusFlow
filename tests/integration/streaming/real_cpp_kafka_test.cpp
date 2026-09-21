#include "kafka_consumer/kafka_consumer_adapter.hpp"

#include <cassert>
#include <chrono>
#include <iostream>
#include <string>
#include <thread>

int main() {

    using namespace nexusflow::streaming;

    KafkaConsumerAdapter kafka(
        "localhost:9092",
        "nexusflow.events",
        "nexusflow-cpp-integration"
    );

    assert(kafka.connect());
    assert(kafka.is_connected());

    std::cout
        << "C++ KAFKA CONNECTION: PASS"
        << std::endl;

    const std::string key =
        "cpp-integration-test";

    const std::string payload =
        "{\"event_id\":\"cpp-001\",\"value\":42}";

    bool produced = false;

    for (int attempt = 0; attempt < 10; ++attempt) {

        if (
            kafka.produce_test_event(
                key,
                payload
            )
        ) {
            produced = true;
            break;
        }

        std::this_thread::sleep_for(
            std::chrono::milliseconds(100)
        );
    }

    assert(produced);

    std::cout
        << "REAL C++ KAFKA PRODUCER: PASS"
        << std::endl;

    KafkaEvent event;

    bool received = false;

    const auto deadline =
        std::chrono::steady_clock::now()
        + std::chrono::seconds(30);

    while (
        std::chrono::steady_clock::now()
        < deadline
    ) {

        if (kafka.poll(event, 1000)) {

            if (
                event.key == key &&
                event.payload == payload
            ) {
                received = true;
                break;
            }
        }
    }

    assert(received);

    std::cout
        << "REAL C++ KAFKA CONSUMER: PASS"
        << std::endl;

    std::cout
        << "EVENT KEY: "
        << event.key
        << std::endl;

    std::cout
        << "EVENT PAYLOAD: "
        << event.payload
        << std::endl;

    kafka.disconnect();

    assert(!kafka.is_connected());

    std::cout
        << "C++ END-TO-END STREAM: PASS"
        << std::endl;

    return 0;
}