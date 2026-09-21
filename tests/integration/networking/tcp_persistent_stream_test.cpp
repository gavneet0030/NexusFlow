#include "../../networking/tcp/cp_stream_session.hpp"
#include "../../networking/protocol/event_protocol.hpp"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <vector>

int main() {

    nexusflow::networking::TcpStreamSession session(
        4096,
        8
    );

    session.start();

    nexusflow::networking::EventMessage message1{};
    message1.version = 1;
    message1.type = 1;
    message1.event_id = 1001;
    message1.timestamp_ms = 100000;

    nexusflow::networking::EventMessage message2{};
    message2.version = 1;
    message2.type = 2;
    message2.event_id = 1002;
    message2.timestamp_ms = 100001;

    std::string encoded1;
    std::string encoded2;

    assert(
        nexusflow::networking::EventProtocol::encode(
            message1,
            encoded1
        )
    );

    assert(
        nexusflow::networking::EventProtocol::encode(
            message2,
            encoded2
        )
    );

    std::vector<std::uint8_t> combined;

    combined.insert(
        combined.end(),
        reinterpret_cast<const std::uint8_t*>(encoded1.data()),
        reinterpret_cast<const std::uint8_t*>(
            encoded1.data() + encoded1.size()
        )
    );

    combined.insert(
        combined.end(),
        reinterpret_cast<const std::uint8_t*>(encoded2.data()),
        reinterpret_cast<const std::uint8_t*>(
            encoded2.data() + encoded2.size()
        )
    );

    assert(
        session.process_bytes(combined)
    );

    session.stop();

    assert(
        session.received_events() == 2
    );

    assert(
        session.rejected_events() == 0
    );

    assert(
        session.processed_events() == 2
    );

    assert(
        session.buffered_bytes() == 0
    );

    std::cout
        << "PERSISTENT STREAM EVENTS: PASS"
        << std::endl;

    std::cout
        << "EVENTS RECEIVED: "
        << session.received_events()
        << std::endl;

    std::cout
        << "EVENTS PROCESSED: "
        << session.processed_events()
        << std::endl;

    std::cout
        << "EVENTS REJECTED: "
        << session.rejected_events()
        << std::endl;

    std::cout
        << "BUFFERED BYTES: "
        << session.buffered_bytes()
        << std::endl;

    std::cout
        << "TCP PERSISTENT STREAM TEST: PASS"
        << std::endl;

    return 0;
}
