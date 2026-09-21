#include "networking/tcp/tcp_persistent_event_server.hpp"
#include "../../networking/protocol/event_protocol.hpp"

#include <cassert>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <thread>
#include <vector>

#ifdef _WIN32
#include <winsock2.h>
#endif

int main() {
#ifdef _WIN32
    constexpr std::uint16_t port = 39111;
    constexpr std::size_t event_count = 1000;

    nexusflow::networking::TcpPersistentEventServer server(
        port,
        4096,
        8
    );

    assert(server.start());

    SOCKET client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    assert(client != INVALID_SOCKET);

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    address.sin_port = htons(port);

    assert(connect(
        client,
        reinterpret_cast<sockaddr*>(&address),
        sizeof(address)
    ) == 0);

    std::vector<std::uint8_t> stream;

    for (std::size_t i = 0; i < event_count; ++i) {
        nexusflow::networking::EventMessage message{};
        message.version = 1;
        message.type = 1;
        message.event_id = 100000 + i;
        message.timestamp_ms =
            static_cast<std::uint64_t>(i);

        std::string encoded;

        assert(
            nexusflow::networking::EventProtocol::encode(message, encoded)
        );

        stream.insert(
            stream.end(),
            encoded.begin(),
            encoded.end()
        );
    }

    const char* bytes =
        reinterpret_cast<const char*>(stream.data());

    std::size_t sent_total = 0;

    while (sent_total < stream.size()) {
        const int sent = send(
            client,
            bytes + sent_total,
            static_cast<int>(stream.size() - sent_total),
            0
        );

        assert(sent > 0);
        sent_total += static_cast<std::size_t>(sent);
    }

    shutdown(client, SD_BOTH);
    closesocket(client);

    for (int i = 0; i < 500; ++i) {
        if (server.processed_events() >= event_count) {
            break;
        }

        std::this_thread::sleep_for(
            std::chrono::milliseconds(10)
        );
    }

    server.stop();

    assert(server.received_events() == event_count);
    assert(server.processed_events() == event_count);
    assert(server.rejected_events() == 0);

    std::cout
        << "PERSISTENT TCP CONNECTION: PASS"
        << std::endl;

    std::cout
        << "EVENTS SENT: "
        << event_count
        << std::endl;

    std::cout
        << "EVENTS RECEIVED: "
        << server.received_events()
        << std::endl;

    std::cout
        << "EVENTS PROCESSED: "
        << server.processed_events()
        << std::endl;

    std::cout
        << "EVENTS REJECTED: "
        << server.rejected_events()
        << std::endl;

    std::cout
        << "TCP PERSISTENT E2E TEST: PASS"
        << std::endl;

    return 0;
#else
    std::cout
        << "TCP PERSISTENT E2E TEST: SKIPPED"
        << std::endl;

    return 0;
#endif
}
