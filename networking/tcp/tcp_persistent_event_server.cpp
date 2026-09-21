#include "tcp_persistent_event_server.hpp"

#include <array>
#include <iostream>

#ifdef _WIN32
#pragma comment(lib, "ws2_32.lib")
#endif

namespace nexusflow::networking {

TcpPersistentEventServer::TcpPersistentEventServer(
    std::uint16_t port,
    std::size_t queue_capacity,
    std::size_t max_workers
)
    : port_(port),
      queue_capacity_(queue_capacity),
      max_workers_(max_workers),
      session_(queue_capacity, max_workers) {
}

TcpPersistentEventServer::~TcpPersistentEventServer() {
    stop();
}

bool TcpPersistentEventServer::start() {
    if (running_.exchange(true)) {
        return true;
    }

#ifdef _WIN32
    WSADATA wsa_data{};

    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
        running_.store(false);
        return false;
    }

    listen_socket_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (listen_socket_ == INVALID_SOCKET) {
        WSACleanup();
        running_.store(false);
        return false;
    }

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    address.sin_port = htons(port_);

    if (bind(
            listen_socket_,
            reinterpret_cast<sockaddr*>(&address),
            sizeof(address)
        ) == SOCKET_ERROR) {
        closesocket(listen_socket_);
        listen_socket_ = INVALID_SOCKET;
        WSACleanup();
        running_.store(false);
        return false;
    }

    if (listen(listen_socket_, SOMAXCONN) == SOCKET_ERROR) {
        closesocket(listen_socket_);
        listen_socket_ = INVALID_SOCKET;
        WSACleanup();
        running_.store(false);
        return false;
    }

    session_.start();
    server_thread_ = std::thread(&TcpPersistentEventServer::run, this);

    return true;
#else
    running_.store(false);
    return false;
#endif
}

void TcpPersistentEventServer::stop() {
    if (!running_.exchange(false)) {
        return;
    }

#ifdef _WIN32
    if (listen_socket_ != INVALID_SOCKET) {
        closesocket(listen_socket_);
        listen_socket_ = INVALID_SOCKET;
    }

    if (server_thread_.joinable()) {
        server_thread_.join();
    }

    session_.stop();

    WSACleanup();
#endif
}

void TcpPersistentEventServer::run() {
#ifdef _WIN32
    while (running_.load()) {
        sockaddr_in client_address{};
        int client_length = sizeof(client_address);

        SOCKET client_socket = accept(
            listen_socket_,
            reinterpret_cast<sockaddr*>(&client_address),
            &client_length
        );

        if (client_socket == INVALID_SOCKET) {
            if (running_.load()) {
                continue;
            }
            break;
        }

        std::array<std::uint8_t, 64 * 1024> receive_buffer{};

        while (running_.load()) {
            const int received = recv(
                client_socket,
                reinterpret_cast<char*>(receive_buffer.data()),
                static_cast<int>(receive_buffer.size()),
                0
            );

            if (received <= 0) {
                break;
            }

            session_.process_bytes(
                receive_buffer.data(),
                static_cast<std::size_t>(received)
            );
        }

        closesocket(client_socket);
    }
#endif
}

std::size_t TcpPersistentEventServer::received_events() const noexcept {
    return session_.received_events();
}

std::size_t TcpPersistentEventServer::rejected_events() const noexcept {
    return session_.rejected_events();
}

std::size_t TcpPersistentEventServer::processed_events() const noexcept {
    return session_.processed_events();
}

} // namespace nexusflow::networking
