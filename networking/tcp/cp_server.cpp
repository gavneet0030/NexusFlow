#include "tcp_server.hpp"

#include <atomic>
#include <cstring>
#include <thread>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")
using socket_t = SOCKET;
constexpr socket_t invalid_socket = INVALID_SOCKET;
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
using socket_t = int;
constexpr socket_t invalid_socket = -1;
#endif

namespace nexusflow {

struct TcpServer::Impl {
    socket_t server_socket = invalid_socket;
    std::uint16_t server_port = 0;

    std::atomic<bool> running{false};
    std::atomic<std::uint64_t> accepted{0};
    std::atomic<std::uint64_t> received{0};

    std::thread worker;
};

TcpServer::TcpServer()
    : impl_(new Impl()) {
}

TcpServer::~TcpServer() {
    stop();
    delete impl_;
}

bool TcpServer::start(std::uint16_t port) {
    if (impl_->running.load()) {
        return false;
    }

#ifdef _WIN32
    WSADATA wsa_data{};

    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
        return false;
    }
#endif

    impl_->server_socket = ::socket(
        AF_INET,
        SOCK_STREAM,
        IPPROTO_TCP
    );

    if (impl_->server_socket == invalid_socket) {
#ifdef _WIN32
        WSACleanup();
#endif
        return false;
    }

    int reuse = 1;

    setsockopt(
        impl_->server_socket,
        SOL_SOCKET,
        SO_REUSEADDR,
        reinterpret_cast<const char*>(&reuse),
        sizeof(reuse)
    );

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    address.sin_port = htons(port);

    if (::bind(
            impl_->server_socket,
            reinterpret_cast<const sockaddr*>(&address),
            sizeof(address)) < 0) {

#ifdef _WIN32
        closesocket(impl_->server_socket);
        WSACleanup();
#else
        close(impl_->server_socket);
#endif

        impl_->server_socket = invalid_socket;
        return false;
    }

    if (::listen(impl_->server_socket, SOMAXCONN) < 0) {

#ifdef _WIN32
        closesocket(impl_->server_socket);
        WSACleanup();
#else
        close(impl_->server_socket);
#endif

        impl_->server_socket = invalid_socket;
        return false;
    }

    impl_->server_port = port;
    impl_->running.store(true);

    impl_->worker = std::thread([this]() {

        while (impl_->running.load()) {

            sockaddr_in client_address{};

#ifdef _WIN32
            int address_length = sizeof(client_address);
#else
            socklen_t address_length = sizeof(client_address);
#endif

            socket_t client = ::accept(
                impl_->server_socket,
                reinterpret_cast<sockaddr*>(&client_address),
                &address_length
            );

            if (client == invalid_socket) {
                if (!impl_->running.load()) {
                    break;
                }

                continue;
            }

            impl_->accepted.fetch_add(1);

            char buffer[4096];

            const int bytes = ::recv(
                client,
                buffer,
                sizeof(buffer),
                0
            );

            if (bytes > 0) {
                impl_->received.fetch_add(1);

                const char response[] = "NEXUSFLOW_ACK";

                ::send(
                    client,
                    response,
                    static_cast<int>(sizeof(response) - 1),
                    0
                );
            }

#ifdef _WIN32
            closesocket(client);
#else
            close(client);
#endif
        }
    });

    return true;
}

void TcpServer::stop() {
    if (!impl_->running.exchange(false)) {
        return;
    }

#ifdef _WIN32
    if (impl_->server_socket != invalid_socket) {
        shutdown(impl_->server_socket, SD_BOTH);
        closesocket(impl_->server_socket);
        impl_->server_socket = invalid_socket;
    }
#else
    if (impl_->server_socket != invalid_socket) {
        shutdown(impl_->server_socket, SHUT_RDWR);
        close(impl_->server_socket);
        impl_->server_socket = invalid_socket;
    }
#endif

    if (impl_->worker.joinable()) {
        impl_->worker.join();
    }

#ifdef _WIN32
    WSACleanup();
#endif
}

bool TcpServer::is_running() const {
    return impl_->running.load();
}

std::uint16_t TcpServer::port() const {
    return impl_->server_port;
}

std::uint64_t TcpServer::connections_accepted() const {
    return impl_->accepted.load();
}

std::uint64_t TcpServer::messages_received() const {
    return impl_->received.load();
}

}
