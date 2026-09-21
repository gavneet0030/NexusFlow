#include "tcp_event_server.hpp"
#include "event_protocol.hpp"

#include <winsock2.h>
#include <ws2tcpip.h>

#include <chrono>
#include <cstring>
#include <iostream>
#include <thread>

#pragma comment(lib, "Ws2_32.lib")

using nexusflow::networking::EventMessage;
using nexusflow::networking::EventProtocol;
using nexusflow::networking::TcpEventServer;

int main() {
    constexpr std::uint16_t port = 39092;

    TcpEventServer server(port);

    if (!server.start()) {
        std::cerr << "SERVER START: FAIL\n";
        return 1;
    }

    std::this_thread::sleep_for(
        std::chrono::milliseconds(100)
    );

    WSADATA wsa_data{};

    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
        server.stop();
        std::cerr << "CLIENT WSA STARTUP: FAIL\n";
        return 1;
    }

    SOCKET client = socket(
        AF_INET,
        SOCK_STREAM,
        IPPROTO_TCP
    );

    if (client == INVALID_SOCKET) {
        WSACleanup();
        server.stop();
        std::cerr << "CLIENT SOCKET: FAIL\n";
        return 1;
    }

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    inet_pton(AF_INET, "127.0.0.1", &address.sin_addr);

    if (connect(
            client,
            reinterpret_cast<sockaddr*>(&address),
            sizeof(address)) == SOCKET_ERROR) {

        closesocket(client);
        WSACleanup();
        server.stop();

        std::cerr << "TCP CONNECTION: FAIL\n";
        return 1;
    }

    std::cout << "TCP CONNECTION: PASS\n";

    EventMessage event;
    event.version = 1;
    event.type = 42;
    event.event_id = 1001;
    event.timestamp_ms = 123456789;
    event.payload = R"({"symbol":"AAPL","price":225.42})";

    std::string encoded;

    if (!EventProtocol::encode(event, encoded)) {
        closesocket(client);
        WSACleanup();
        server.stop();

        std::cerr << "EVENT ENCODE: FAIL\n";
        return 1;
    }

    int sent = send(
        client,
        encoded.data(),
        static_cast<int>(encoded.size()),
        0
    );

    if (sent != static_cast<int>(encoded.size())) {
        closesocket(client);
        WSACleanup();
        server.stop();

        std::cerr << "EVENT SEND: FAIL\n";
        return 1;
    }

    char response[128]{};

    int received = recv(
        client,
        response,
        sizeof(response) - 1,
        0
    );

    closesocket(client);
    WSACleanup();

    if (received <= 0) {
        server.stop();
        std::cerr << "EVENT ACK: FAIL\n";
        return 1;
    }

    response[received] = '\0';

    if (std::string(response) != "NEXUSFLOW_EVENT_ACK") {
        server.stop();
        std::cerr << "EVENT ACK: FAIL\n";
        return 1;
    }

    std::cout << "EVENT SEND: PASS\n";
    std::cout << "EVENT ACK: PASS\n";

    for (int i = 0; i < 20; ++i) {
        if (server.received_events() == 1) {
            break;
        }

        std::this_thread::sleep_for(
            std::chrono::milliseconds(10)
        );
    }

    if (server.received_events() != 1) {
        server.stop();
        std::cerr << "SERVER EVENT COUNT: FAIL\n";
        return 1;
    }

    if (server.rejected_events() != 0) {
        server.stop();
        std::cerr << "SERVER REJECTION COUNT: FAIL\n";
        return 1;
    }

    server.stop();

    std::cout << "SERVER EVENT COUNT: PASS\n";
    std::cout << "TCP EVENT PROTOCOL INTEGRATION TEST: PASS\n";

    return 0;

    const int multi_event_count = 100;

    for (int i = 0; i < multi_event_count; ++i) {
        EventMessage multi_event{};
        multi_event.version = EventProtocol::kVersion;
        multi_event.type = 1;
        multi_event.event_id = static_cast<std::uint64_t>(1000 + i);
        multi_event.timestamp_ms =
            static_cast<std::uint64_t>(2000000 + i);
        multi_event.payload = "1.0";

        std::string multi_encoded;

        if (!EventProtocol::encode(multi_event, multi_encoded)) {
            std::cerr << "MULTI EVENT ENCODE: FAIL" << std::endl;
            server.stop();
            return 1;
        }

        SOCKET multi_socket = socket(
            AF_INET,
            SOCK_STREAM,
            IPPROTO_TCP
        );

        if (multi_socket == INVALID_SOCKET) {
            std::cerr << "MULTI EVENT SOCKET: FAIL" << std::endl;
            server.stop();
            return 1;
        }

        sockaddr_in multi_address{};
        multi_address.sin_family = AF_INET;
        multi_address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        multi_address.sin_port = htons(port);

        if (connect(
                multi_socket,
                reinterpret_cast<sockaddr*>(&multi_address),
                sizeof(multi_address)) == SOCKET_ERROR) {

            closesocket(multi_socket);
            std::cerr << "MULTI EVENT CONNECTION: FAIL" << std::endl;
            server.stop();
            return 1;
        }

        int sent = send(
            multi_socket,
            multi_encoded.data(),
            static_cast<int>(multi_encoded.size()),
            0
        );

        if (sent != static_cast<int>(multi_encoded.size())) {
            closesocket(multi_socket);
            std::cerr << "MULTI EVENT SEND: FAIL" << std::endl;
            server.stop();
            return 1;
        }

        char acknowledgement[64]{};

        int acknowledgement_size = recv(
            multi_socket,
            acknowledgement,
            static_cast<int>(sizeof(acknowledgement) - 1),
            0
        );

        closesocket(multi_socket);

        if (acknowledgement_size <= 0) {
            std::cerr << "MULTI EVENT ACK: FAIL" << std::endl;
            server.stop();
            return 1;
        }
    }

    const auto multi_deadline =
        std::chrono::steady_clock::now() +
        std::chrono::seconds(5);

    while (server.received_events() < static_cast<std::uint64_t>(multi_event_count + 1) &&
           std::chrono::steady_clock::now() < multi_deadline) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    if (server.received_events() !=
        static_cast<std::uint64_t>(multi_event_count + 1)) {

        std::cerr << "TCP MULTI EVENT PIPELINE TEST: FAIL" << std::endl;
        server.stop();
        return 1;
    }

    const auto processed_deadline =
        std::chrono::steady_clock::now() +
        std::chrono::seconds(5);

    while (server.processed_events() <
               static_cast<std::uint64_t>(multi_event_count + 1) &&
           std::chrono::steady_clock::now() < processed_deadline) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    if (server.processed_events() !=
        static_cast<std::uint64_t>(multi_event_count + 1)) {

        std::cerr << "TCP MULTI EVENT PROCESSING: FAIL" << std::endl;
        server.stop();
        return 1;
    }

    std::cout << "TCP MULTI EVENT PIPELINE TEST: PASS" << std::endl;}