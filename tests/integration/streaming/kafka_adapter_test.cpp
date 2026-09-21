#include "kafka_consumer/kafka_consumer_adapter.hpp"

#include <cassert>
#include <iostream>

int main() {
    using namespace nexusflow::streaming;

    KafkaConsumerAdapter consumer(
        "localhost:9092",
        "nexusflow.events",
        "nexusflow"
    );

    assert(consumer.connect());
    assert(consumer.is_connected());

    KafkaEvent event;

    assert(
        consumer.decode_message(
            "event-1",
            "sample-payload",
            0,
            42,
            123456789,
            event
        )
    );

    assert(event.key == "event-1");
    assert(event.payload == "sample-payload");
    assert(event.partition == 0);
    assert(event.offset == 42);
    assert(event.timestamp_ms == 123456789);

    assert(
        !consumer.decode_message(
            "event-2",
            "",
            0,
            43,
            123456790,
            event
        )
    );

    consumer.disconnect();

    assert(!consumer.is_connected());

    std::cout << "KAFKA ADAPTER TEST: PASS" << std::endl;

    return 0;
}