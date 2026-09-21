#include <chrono>
#include <cstring>
#include <iostream>
#include <thread>

#include "tcp_server.hpp"

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")
#endif

int main() {

    nexusflow::TcpServer server;

    const std::uint16_t port = 39091;

    if (!server.start(port)) {
        std::cerr << "TCP server start failed." << std::endl;
        return 1;
    }

    std::this_thread::sleep_for(
        std::chrono::milliseconds(100)
    );

#ifdef _WIN32
    WSADATA wsa_data{};

    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
        server.stop();
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
        return 1;
    }

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    inet_pton(
        AF_INET,
        "127.0.0.1",
        &address.sin_addr
    );

    if (connect(
            client,
            reinterpret_cast<const sockaddr*>(&address),
            sizeof(address)) != 0) {

        closesocket(client);
        WSACleanup();
        server.stop();
        return 1;
    }

    const char message[] = "NEXUSFLOW_TEST_EVENT";

    send(
        client,
        message,
        static_cast<int>(sizeof(message) - 1),
        0
    );

    char response[64]{};

    const int received = recv(
        client,
        response,
        sizeof(response) - 1,
        0
    );

    closesocket(client);
    WSACleanup();

    if (received <= 0) {
        server.stop();
        return 1;
    }

#else
    return 1;
#endif

    server.stop();

    if (server.connections_accepted() != 1) {
        std::cerr << "TCP connection count mismatch." << std::endl;
        return 1;
    }

    if (server.messages_received() != 1) {
        std::cerr << "TCP message count mismatch." << std::endl;
        return 1;
    }

    std::cout << "TCP CONNECTION: PASS" << std::endl;
    std::cout << "TCP MESSAGE: PASS" << std::endl;
    std::cout << "TCP INTEGRATION TEST: PASS" << std::endl;

    return 0;
}
