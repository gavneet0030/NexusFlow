#include "tcp_core_pipeline.hpp"
#include "event_protocol.hpp"

#include <iostream>
#include <string>

using nexusflow::Event;
using nexusflow::networking::EventMessage;
using nexusflow::networking::EventProtocol;
using nexusflow::networking::TcpCorePipeline;

int main() {
    TcpCorePipeline pipeline(16);

    EventMessage message;

    message.version = 1;
    message.type = 42;
    message.event_id = 5001;
    message.timestamp_ms = 123456789;
    message.payload = R"({"symbol":"AAPL","price":225.42})";

    std::string encoded;

    if (!EventProtocol::encode(message, encoded)) {
        std::cerr << "EVENT ENCODE: FAIL\n";
        return 1;
    }

    if (!pipeline.submit_encoded_event(encoded)) {
        std::cerr << "QUEUE SUBMISSION: FAIL\n";
        return 1;
    }

    std::cout << "QUEUE SUBMISSION: PASS\n";

    Event event;

    if (!pipeline.try_pop_event(event)) {
        std::cerr << "QUEUE POP: FAIL\n";
        return 1;
    }

    if (event.id != message.event_id) {
        std::cerr << "EVENT MAPPING: FAIL\n";
        return 1;
    }

    std::cout << "QUEUE POP: PASS\n";
    std::cout << "EVENT MAPPING: PASS\n";

    if (pipeline.accepted_events() != 1) {
        std::cerr << "ACCEPTED COUNT: FAIL\n";
        return 1;
    }

    std::cout << "ACCEPTED COUNT: PASS\n";

    std::string invalid = encoded;
    invalid.resize(invalid.size() - 1);

    if (pipeline.submit_encoded_event(invalid)) {
        std::cerr << "INVALID EVENT: FAIL\n";
        return 1;
    }

    if (pipeline.rejected_events() != 1) {
        std::cerr << "REJECTED COUNT: FAIL\n";
        return 1;
    }

    std::cout << "INVALID EVENT: PASS\n";
    std::cout << "REJECTED COUNT: PASS\n";

    std::cout << "TCP CORE PIPELINE TEST: PASS\n";

    return 0;
}