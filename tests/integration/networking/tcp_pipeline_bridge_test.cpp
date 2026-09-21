#include "tcp_pipeline_bridge.hpp"
#include "event_protocol.hpp"

#include <iostream>
#include <string>

using nexusflow::networking::EventMessage;
using nexusflow::networking::EventProtocol;
using nexusflow::networking::TcpPipelineBridge;

int main() {
    TcpPipelineBridge bridge;

    EventMessage event;
    event.version = 1;
    event.type = 100;
    event.event_id = 9001;
    event.timestamp_ms = 123456789;
    event.payload = R"({"symbol":"AAPL","price":225.42})";

    std::string encoded;

    if (!EventProtocol::encode(event, encoded)) {
        std::cerr << "EVENT ENCODE: FAIL\n";
        return 1;
    }

    if (!bridge.submit_encoded_event(encoded)) {
        std::cerr << "PIPELINE SUBMISSION: FAIL\n";
        return 1;
    }

    if (bridge.accepted_events() != 1) {
        std::cerr << "ACCEPTED EVENT COUNT: FAIL\n";
        return 1;
    }

    std::cout << "PIPELINE SUBMISSION: PASS\n";
    std::cout << "ACCEPTED EVENT COUNT: PASS\n";

    std::string invalid = encoded;
    invalid.resize(invalid.size() - 1);

    if (bridge.submit_encoded_event(invalid)) {
        std::cerr << "INVALID EVENT REJECTION: FAIL\n";
        return 1;
    }

    if (bridge.rejected_events() != 1) {
        std::cerr << "REJECTED EVENT COUNT: FAIL\n";
        return 1;
    }

    std::cout << "INVALID EVENT REJECTION: PASS\n";
    std::cout << "REJECTED EVENT COUNT: PASS\n";
    std::cout << "TCP PIPELINE BRIDGE TEST: PASS\n";

    return 0;
}