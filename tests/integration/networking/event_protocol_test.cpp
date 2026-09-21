#include "event_protocol.hpp"

#include <iostream>
#include <string>

using nexusflow::networking::EventMessage;
using nexusflow::networking::EventProtocol;

int main() {
    EventMessage original;

    original.version = 1;
    original.type = 7;
    original.event_id = 123456789;
    original.timestamp_ms = 987654321;
    original.payload = R"({"symbol":"AAPL","price":225.42})";

    std::string encoded;

    if (!EventProtocol::encode(original, encoded)) {
        std::cerr << "ENCODE: FAIL\n";
        return 1;
    }

    std::cout << "ENCODE: PASS\n";

    EventMessage decoded;

    if (!EventProtocol::decode(encoded, decoded)) {
        std::cerr << "DECODE: FAIL\n";
        return 1;
    }

    if (
        decoded.version != original.version ||
        decoded.type != original.type ||
        decoded.event_id != original.event_id ||
        decoded.timestamp_ms != original.timestamp_ms ||
        decoded.payload != original.payload
    ) {
        std::cerr << "ROUND TRIP: FAIL\n";
        return 1;
    }

    std::cout << "ROUND TRIP: PASS\n";

    std::string invalid = encoded;
    invalid.resize(invalid.size() - 1);

    EventMessage rejected;

    if (EventProtocol::decode(invalid, rejected)) {
        std::cerr << "INVALID MESSAGE REJECTION: FAIL\n";
        return 1;
    }

    std::cout << "INVALID MESSAGE REJECTION: PASS\n";
    std::cout << "EVENT PROTOCOL INTEGRATION TEST: PASS\n";

    return 0;
}