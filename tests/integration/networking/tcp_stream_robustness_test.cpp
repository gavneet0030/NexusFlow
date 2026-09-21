#include "../../networking/tcp/cp_stream_session.hpp"
#include "../../networking/protocol/event_protocol.hpp"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

static std::vector<std::uint8_t> to_bytes(
    const std::string& value
) {
    return std::vector<std::uint8_t>(
        reinterpret_cast<const std::uint8_t*>(value.data()),
        reinterpret_cast<const std::uint8_t*>(
            value.data() + value.size()
        )
    );
}

int main() {

    nexusflow::networking::EventMessage message1{};
    message1.version = 1;
    message1.type = 1;
    message1.event_id = 5001;
    message1.timestamp_ms = 1000;
    message1.payload = "event-one";
    message1.payload_size =
        static_cast<std::uint32_t>(message1.payload.size());

    nexusflow::networking::EventMessage message2{};
    message2.version = 1;
    message2.type = 2;
    message2.event_id = 5002;
    message2.timestamp_ms = 1001;
    message2.payload = "event-two";
    message2.payload_size =
        static_cast<std::uint32_t>(message2.payload.size());

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

    std::vector<std::uint8_t> bytes1 = to_bytes(encoded1);
    std::vector<std::uint8_t> bytes2 = to_bytes(encoded2);

    nexusflow::networking::TcpStreamSession session(
        4096,
        8
    );

    session.start();

    // --------------------------------------------------------
    // Test 1: Partial frame
    // --------------------------------------------------------

    const std::size_t split =
        bytes1.size() / 2;

    assert(
        session.process_bytes(
            bytes1.data(),
            split
        )
    );

    assert(
        session.received_events() == 0
    );

    assert(
        session.buffered_bytes() == split
    );

    assert(
        session.process_bytes(
            bytes1.data() + split,
            bytes1.size() - split
        )
    );

    assert(
        session.received_events() == 1
    );

    // --------------------------------------------------------
    // Test 2: Two frames in one TCP receive
    // --------------------------------------------------------

    std::vector<std::uint8_t> combined;

    combined.insert(
        combined.end(),
        bytes1.begin(),
        bytes1.end()
    );

    combined.insert(
        combined.end(),
        bytes2.begin(),
        bytes2.end()
    );

    assert(
        session.process_bytes(combined)
    );

    assert(
        session.received_events() == 3
    );

    // --------------------------------------------------------
    // Test 3: Invalid frame must not crash
    // --------------------------------------------------------

    std::vector<std::uint8_t> invalid(
        32,
        0xFF
    );

    session.process_bytes(invalid);

    // --------------------------------------------------------
    // Finish
    // --------------------------------------------------------

    session.stop();

    assert(
        session.processed_events() == 3
    );


    std::cout
        << "PARTIAL FRAME TEST: PASS"
        << std::endl;

    std::cout
        << "COALESCED FRAME TEST: PASS"
        << std::endl;

    std::cout
        << "MALFORMED FRAME TEST: PASS"
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
        << "TCP STREAM ROBUSTNESS TEST: PASS"
        << std::endl;

    return 0;
}
