#include "tcp_event_server.hpp"
#include "event_protocol.hpp"

#include <winsock2.h>
#include <ws2tcpip.h>

#include <chrono>
#include <cstdint>
#include <iostream>
#include <string>
#include <thread>

#pragma comment(lib, "Ws2_32.lib")

using nexusflow::networking::EventMessage;
using nexusflow::networking::EventProtocol;
using nexusflow::networking::TcpEventServer;

int main() {
    constexpr std::uint16_t port = 39100;
    constexpr int event_count = 1000;

    TcpEventServer server(port, 4096, 8);

    if (!server.start()) {
        std::cerr << "SERVER START: FAIL" << std::endl;
        return 1;
    }

    std::cout << "SERVER START: PASS" << std::endl;

    const auto start_time = std::chrono::steady_clock::now();

    for (int i = 0; i < event_count; ++i) {
        EventMessage event{};
        event.version = EventProtocol::kVersion;
        event.type = 1;
        event.event_id = static_cast<std::uint64_t>(100000 + i);
        event.timestamp_ms =
            static_cast<std::uint64_t>(3000000 + i);
        event.payload = "1.0";

        std::string encoded;

        if (!EventProtocol::encode(event, encoded)) {
            std::cerr << "EVENT ENCODE: FAIL" << std::endl;
            server.stop();
            return 1;
        }

        SOCKET client = socket(
            AF_INET,
            SOCK_STREAM,
            IPPROTO_TCP
        );

        if (client == INVALID_SOCKET) {
            std::cerr << "CLIENT SOCKET: FAIL" << std::endl;
            server.stop();
            return 1;
        }

        sockaddr_in address{};
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        address.sin_port = htons(port);

        if (connect(
                client,
                reinterpret_cast<sockaddr*>(&address),
                sizeof(address)) == SOCKET_ERROR) {

            closesocket(client);
            std::cerr << "CLIENT CONNECT: FAIL" << std::endl;
            server.stop();
            return 1;
        }

        const int sent = send(
            client,
            encoded.data(),
            static_cast<int>(encoded.size()),
            0
        );

        if (sent != static_cast<int>(encoded.size())) {
            closesocket(client);
            std::cerr << "EVENT SEND: FAIL" << std::endl;
            server.stop();
            return 1;
        }

        char ack[64]{};

        const int received = recv(
            client,
            ack,
            static_cast<int>(sizeof(ack) - 1),
            0
        );

        closesocket(client);

        if (received <= 0) {
            std::cerr << "EVENT ACK: FAIL" << std::endl;
            server.stop();
            return 1;
        }
    }

    const auto receive_deadline =
        std::chrono::steady_clock::now() +
        std::chrono::seconds(10);

    while (server.received_events() <
               static_cast<std::uint64_t>(event_count) &&
           std::chrono::steady_clock::now() < receive_deadline) {

        std::this_thread::sleep_for(
            std::chrono::milliseconds(10)
        );
    }

    const auto process_deadline =
        std::chrono::steady_clock::now() +
        std::chrono::seconds(10);

    while (server.processed_events() <
               static_cast<std::uint64_t>(event_count) &&
           std::chrono::steady_clock::now() < process_deadline) {

        std::this_thread::sleep_for(
            std::chrono::milliseconds(10)
        );
    }

    const auto end_time = std::chrono::steady_clock::now();

    const double elapsed_seconds =
        std::chrono::duration<double>(
            end_time - start_time
        ).count();

    const double throughput =
        elapsed_seconds > 0.0
            ? static_cast<double>(server.processed_events()) /
                  elapsed_seconds
            : 0.0;

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
        << "ELAPSED_SECONDS: "
        << elapsed_seconds
        << std::endl;

    std::cout
        << "END_TO_END_THROUGHPUT_EPS: "
        << throughput
        << std::endl;

    if (server.received_events() !=
            static_cast<std::uint64_t>(event_count) ||
        server.processed_events() !=
            static_cast<std::uint64_t>(event_count) ||
        server.rejected_events() != 0) {

        std::cerr
            << "TCP HIGH-LOAD E2E: FAIL"
            << std::endl;

        server.stop();
        return 1;
    }

    std::cout
        << "TCP HIGH-LOAD E2E: PASS"
        << std::endl;

    server.stop();

    return 0;
}