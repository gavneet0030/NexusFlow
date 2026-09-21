#include "tcp_event_server.hpp"

#include "event_protocol.hpp"

#include <winsock2.h>
#include <ws2tcpip.h>

#include <cstring>
#include <string>

#pragma comment(lib, "Ws2_32.lib")

namespace nexusflow::networking {

TcpEventServer::TcpEventServer(std::uint16_t port)
    : TcpEventServer(port, 4096, 8) {
}
TcpEventServer::TcpEventServer(
    std::uint16_t port,
    std::size_t queue_capacity,
    std::size_t max_workers)
    : port_(port),
      pipeline_(queue_capacity, max_workers) {
}

TcpEventServer::~TcpEventServer() {
    stop();
}

bool TcpEventServer::start() {
    if (running_.load()) {
        return false;
    }

    WSADATA wsa_data{};

    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
        return false;
    }

    SOCKET server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (server_socket == INVALID_SOCKET) {
        pipeline_.drain();
    pipeline_.shutdown();

    WSACleanup();
        return false;
    }

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    address.sin_port = htons(port_);

    if (bind(
            server_socket,
            reinterpret_cast<sockaddr*>(&address),
            sizeof(address)) == SOCKET_ERROR) {

        closesocket(server_socket);
        pipeline_.drain();
    pipeline_.shutdown();

    WSACleanup();
        return false;
    }

    if (listen(server_socket, SOMAXCONN) == SOCKET_ERROR) {
        closesocket(server_socket);
        pipeline_.drain();
    pipeline_.shutdown();

    WSACleanup();
        return false;
    }

    server_socket_ =
        static_cast<std::uintptr_t>(server_socket);

    pipeline_.start();
    running_.store(true);

    worker_ = std::thread(&TcpEventServer::run, this);

    return true;
}

void TcpEventServer::stop() {
    if (!running_.exchange(false)) {
        return;
    }

    SOCKET server_socket =
        static_cast<SOCKET>(server_socket_);

    if (server_socket != INVALID_SOCKET) {
        closesocket(server_socket);
        server_socket_ = 0;
    }

    if (worker_.joinable()) {
        worker_.join();
    }

    pipeline_.drain();
    pipeline_.shutdown();

    WSACleanup();
}

bool TcpEventServer::is_running() const {
    return running_.load();
}

std::uint64_t TcpEventServer::received_events() const {
    return received_events_.load();
}

std::uint64_t TcpEventServer::rejected_events() const {
    return rejected_events_.load();
}

std::uint64_t TcpEventServer::processed_events() const {
    return pipeline_.metrics().processed;
}

void TcpEventServer::run() {
    SOCKET server_socket =
        static_cast<SOCKET>(server_socket_);

    while (running_.load()) {
        sockaddr_in client_address{};
        int client_size = sizeof(client_address);

        SOCKET client_socket = accept(
            server_socket,
            reinterpret_cast<sockaddr*>(&client_address),
            &client_size
        );

        if (client_socket == INVALID_SOCKET) {
            if (running_.load()) {
                continue;
            }

            break;
        }

        std::string buffer;
        buffer.resize(1024 * 1024 + 64);

        int received = recv(
            client_socket,
            buffer.data(),
            static_cast<int>(buffer.size()),
            0
        );

        if (received > 0) {
            buffer.resize(static_cast<std::size_t>(received));

            EventMessage event;

            if (EventProtocol::decode(buffer, event)) {
                if (pipeline_.submit(event)) {
                    received_events_.fetch_add(1);

                    const char acknowledgement[] =
                        "NEXUSFLOW_EVENT_ACK";

                    send(
                        client_socket,
                        acknowledgement,
                        static_cast<int>(sizeof(acknowledgement) - 1),
                        0
                    );
                }
                else {
                    rejected_events_.fetch_add(1);

                    const char rejection[] =
                        "NEXUSFLOW_EVENT_REJECT";

                    send(
                        client_socket,
                        rejection,
                        static_cast<int>(sizeof(rejection) - 1),
                        0
                    );
                }
            }
            else {
                rejected_events_.fetch_add(1);

                const char rejection[] =
                    "NEXUSFLOW_EVENT_REJECT";

                send(
                    client_socket,
                    rejection,
                    static_cast<int>(sizeof(rejection) - 1),
                    0
                );
            }
        }

        closesocket(client_socket);
    }
}

}