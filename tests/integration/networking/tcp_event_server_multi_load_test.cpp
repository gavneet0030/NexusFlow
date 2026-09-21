#include "tcp_event_server.hpp"
#include "event_protocol.hpp"

#include <winsock2.h>
#include <ws2tcpip.h>

#include <chrono>
#include <cstdint>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#pragma comment(lib, "Ws2_32.lib")

using nexusflow::networking::EventMessage;
using nexusflow::networking::EventProtocol;
using nexusflow::networking::TcpEventServer;

static bool send_events(
    std::uint16_t port,
    int event_count,
    std::uint64_t id_base) {

    for (int i = 0; i < event_count; ++i) {
        EventMessage event{};
        event.version = EventProtocol::kVersion;
        event.type = 1;
        event.event_id = id_base + static_cast<std::uint64_t>(i);
        event.timestamp_ms =
            4000000ULL + static_cast<std::uint64_t>(i);
        event.payload = "1.0";

        std::string encoded;

        if (!EventProtocol::encode(event, encoded)) {
            return false;
        }

        SOCKET client = socket(
            AF_INET,
            SOCK_STREAM,
            IPPROTO_TCP
        );

        if (client == INVALID_SOCKET) {
            return false;
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
            return false;
        }

        const int sent = send(
            client,
            encoded.data(),
            static_cast<int>(encoded.size()),
            0
        );

        if (sent != static_cast<int>(encoded.size())) {
            closesocket(client);
            return false;
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
            return false;
        }
    }

    return true;
}

static bool wait_for_processed(
    const TcpEventServer& server,
    std::uint64_t expected,
    int timeout_seconds) {

    const auto deadline =
        std::chrono::steady_clock::now() +
        std::chrono::seconds(timeout_seconds);

    while (server.processed_events() < expected &&
           std::chrono::steady_clock::now() < deadline) {

        std::this_thread::sleep_for(
            std::chrono::milliseconds(10)
        );
    }

    return server.processed_events() == expected;
}

int main() {
    constexpr std::uint16_t port = 39101;

    const std::vector<int> loads = {
        100,
        500,
        1000,
        2500
    };

    for (int event_count : loads) {
        TcpEventServer server(port, 4096, 8);

        if (!server.start()) {
            std::cerr << "SERVER START: FAIL" << std::endl;
            return 1;
        }

        const auto start_time =
            std::chrono::steady_clock::now();

        const bool send_ok =
            send_events(
                port,
                event_count,
                static_cast<std::uint64_t>(event_count) * 100000ULL
            );

        if (!send_ok) {
            std::cerr
                << "LOAD "
                << event_count
                << " SEND: FAIL"
                << std::endl;

            server.stop();
            return 1;
        }

        const bool processed_ok =
            wait_for_processed(
                server,
                static_cast<std::uint64_t>(event_count),
                15
            );

        const auto end_time =
            std::chrono::steady_clock::now();

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
            << "LOAD="
            << event_count
            << " RECEIVED="
            << server.received_events()
            << " PROCESSED="
            << server.processed_events()
            << " REJECTED="
            << server.rejected_events()
            << " ELAPSED_SECONDS="
            << elapsed_seconds
            << " THROUGHPUT_EPS="
            << throughput
            << std::endl;

        if (!processed_ok ||
            server.received_events() !=
                static_cast<std::uint64_t>(event_count) ||
            server.rejected_events() != 0) {

            std::cerr
                << "LOAD "
                << event_count
                << ": FAIL"
                << std::endl;

            server.stop();
            return 1;
        }

        std::cout
            << "LOAD "
            << event_count
            << ": PASS"
            << std::endl;

        server.stop();
    }

    std::cout
        << "TCP MULTI-LOAD E2E: PASS"
        << std::endl;

    return 0;
}