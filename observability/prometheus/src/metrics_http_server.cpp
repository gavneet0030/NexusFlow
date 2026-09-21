#include "metrics_http_server.hpp"

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

#include <cstring>
#include <iostream>
#include <string>

namespace nexusflow::observability {

namespace {

#ifdef _WIN32

SOCKET to_socket(std::uintptr_t value) {
    return static_cast<SOCKET>(value);
}

void close_socket(SOCKET socket) {
    closesocket(socket);
}

#else

int to_socket(std::uintptr_t value) {
    return static_cast<int>(value);
}

void close_socket(int socket) {
    close(socket);
}

#endif

}

MetricsHttpServer::MetricsHttpServer(
    PrometheusMetrics& metrics,
    std::uint16_t port
)
    : metrics_(metrics),
      port_(port) {
}

MetricsHttpServer::~MetricsHttpServer() {
    stop();
}

bool MetricsHttpServer::start() {
    if (running_.load(std::memory_order_acquire)) {
        return true;
    }

#ifdef _WIN32
    WSADATA data{};

    if (WSAStartup(MAKEWORD(2, 2), &data) != 0) {
        return false;
    }
#endif

#ifdef _WIN32
    SOCKET server_socket = socket(
        AF_INET,
        SOCK_STREAM,
        IPPROTO_TCP
    );

    if (server_socket == INVALID_SOCKET) {
        WSACleanup();
        return false;
    }
#else
    int server_socket = socket(
        AF_INET,
        SOCK_STREAM,
        0
    );

    if (server_socket < 0) {
        return false;
    }
#endif

    int reuse = 1;

#ifdef _WIN32
    setsockopt(
        server_socket,
        SOL_SOCKET,
        SO_REUSEADDR,
        reinterpret_cast<const char*>(&reuse),
        sizeof(reuse)
    );
#else
    setsockopt(
        server_socket,
        SOL_SOCKET,
        SO_REUSEADDR,
        &reuse,
        sizeof(reuse)
    );
#endif

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(port_);

    if (bind(
            server_socket,
            reinterpret_cast<const sockaddr*>(&address),
            sizeof(address)) != 0) {

        close_socket(server_socket);

#ifdef _WIN32
        WSACleanup();
#endif

        return false;
    }

    if (listen(server_socket, 16) != 0) {
        close_socket(server_socket);

#ifdef _WIN32
        WSACleanup();
#endif

        return false;
    }

    socket_handle_ =
        static_cast<std::uintptr_t>(server_socket);

    running_.store(true, std::memory_order_release);

    thread_ = std::thread(&MetricsHttpServer::run, this);

    return true;
}

void MetricsHttpServer::stop() {
    const bool was_running =
        running_.exchange(false, std::memory_order_acq_rel);

    if (!was_running) {
        return;
    }

#ifdef _WIN32
    SOCKET server_socket = to_socket(socket_handle_);

    if (server_socket != INVALID_SOCKET) {
        shutdown(server_socket, SD_BOTH);
        close_socket(server_socket);
    }
#else
    int server_socket = to_socket(socket_handle_);

    if (server_socket >= 0) {
        shutdown(server_socket, SHUT_RDWR);
        close_socket(server_socket);
    }
#endif

    socket_handle_ = 0;

    if (thread_.joinable()) {
        thread_.join();
    }

#ifdef _WIN32
    WSACleanup();
#endif
}

bool MetricsHttpServer::running() const {
    return running_.load(std::memory_order_acquire);
}

std::uint16_t MetricsHttpServer::port() const {
    return port_;
}

void MetricsHttpServer::run() {
#ifdef _WIN32
    SOCKET server_socket = to_socket(socket_handle_);
#else
    int server_socket = to_socket(socket_handle_);
#endif

    while (running_.load(std::memory_order_acquire)) {

        sockaddr_in client_address{};
#ifdef _WIN32
        int client_length = sizeof(client_address);

        SOCKET client_socket = accept(
            server_socket,
            reinterpret_cast<sockaddr*>(&client_address),
            &client_length
        );

        if (client_socket == INVALID_SOCKET) {
            if (running_.load(std::memory_order_acquire)) {
                continue;
            }
            break;
        }
#else
        socklen_t client_length = sizeof(client_address);

        int client_socket = accept(
            server_socket,
            reinterpret_cast<sockaddr*>(&client_address),
            &client_length
        );

        if (client_socket < 0) {
            if (running_.load(std::memory_order_acquire)) {
                continue;
            }
            break;
        }
#endif

        char request[4096]{};

#ifdef _WIN32
        const int received = recv(
            client_socket,
            request,
            sizeof(request) - 1,
            0
        );
#else
        const int received = static_cast<int>(recv(
            client_socket,
            request,
            sizeof(request) - 1,
            0
        ));
#endif

        if (received <= 0) {
            close_socket(client_socket);
            continue;
        }

        request[received] = '\0';

        const std::string request_text(request);

        const bool metrics_request =
            request_text.find("GET /metrics") == 0;

        std::string body;
        std::string response;

        if (metrics_request) {
            body = metrics_.render();

            response =
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: text/plain; version=0.0.4; charset=utf-8\r\n"
                "Content-Length: " +
                std::to_string(body.size()) +
                "\r\n"
                "Connection: close\r\n"
                "\r\n" +
                body;
        } else {
            body =
                "NexusFlow metrics exporter\n"
                "Use GET /metrics\n";

            response =
                "HTTP/1.1 404 Not Found\r\n"
                "Content-Type: text/plain; charset=utf-8\r\n"
                "Content-Length: " +
                std::to_string(body.size()) +
                "\r\n"
                "Connection: close\r\n"
                "\r\n" +
                body;
        }

#ifdef _WIN32
        send(
            client_socket,
            response.data(),
            static_cast<int>(response.size()),
            0
        );
#else
        send(
            client_socket,
            response.data(),
            response.size(),
            0
        );
#endif

        close_socket(client_socket);
    }
}

}
