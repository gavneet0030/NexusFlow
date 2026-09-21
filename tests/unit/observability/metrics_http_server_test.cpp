#include "metrics_http_server.hpp"

#include <chrono>
#include <iostream>
#include <string>
#include <thread>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

int main() {
    using namespace nexusflow::observability;

    PrometheusMetrics metrics;

    metrics.record_received(1000);
    metrics.record_processed(995);
    metrics.record_rejected(5);
    metrics.set_active_workers(4);
    metrics.set_queue_depth(7);
    metrics.observe_latency_us(25.0);

    MetricsHttpServer server(metrics, 19100);

    if (!server.start()) {
        std::cerr << "HTTP SERVER START: FAIL" << std::endl;
        return 1;
    }

    if (!server.running()) {
        std::cerr << "HTTP SERVER STATE: FAIL" << std::endl;
        server.stop();
        return 2;
    }

    std::this_thread::sleep_for(
        std::chrono::milliseconds(100)
    );

#ifdef _WIN32
    WSADATA data{};

    if (WSAStartup(MAKEWORD(2, 2), &data) != 0) {
        server.stop();
        return 3;
    }
#endif

#ifdef _WIN32
    SOCKET client = socket(
        AF_INET,
        SOCK_STREAM,
        IPPROTO_TCP
    );

    if (client == INVALID_SOCKET) {
#ifdef _WIN32
        WSACleanup();
#endif
        server.stop();
        return 4;
    }
#else
    int client = socket(AF_INET, SOCK_STREAM, 0);

    if (client < 0) {
        server.stop();
        return 4;
    }
#endif

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(19100);
    inet_pton(AF_INET, "127.0.0.1", &address.sin_addr);

    if (connect(
            client,
            reinterpret_cast<const sockaddr*>(&address),
            sizeof(address)) != 0) {

#ifdef _WIN32
        closesocket(client);
        WSACleanup();
#else
        close(client);
#endif

        server.stop();
        return 5;
    }

    const std::string request =
        "GET /metrics HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Connection: close\r\n"
        "\r\n";

#ifdef _WIN32
    send(
        client,
        request.data(),
        static_cast<int>(request.size()),
        0
    );
#else
    send(
        client,
        request.data(),
        request.size(),
        0
    );
#endif

    std::string response;
    char buffer[4096];

    while (true) {
#ifdef _WIN32
        const int count = recv(
            client,
            buffer,
            sizeof(buffer),
            0
        );
#else
        const int count = static_cast<int>(recv(
            client,
            buffer,
            sizeof(buffer),
            0
        ));
#endif

        if (count <= 0) {
            break;
        }

        response.append(buffer, count);
    }

#ifdef _WIN32
    closesocket(client);
    WSACleanup();
#else
    close(client);
#endif

    server.stop();

    if (response.find("HTTP/1.1 200 OK") == std::string::npos) {
        std::cerr << "HTTP STATUS: FAIL" << std::endl;
        return 6;
    }

    if (response.find(
            "nexusflow_events_received_total 1000")
        == std::string::npos) {
        std::cerr << "RECEIVED METRIC: FAIL" << std::endl;
        return 7;
    }

    if (response.find(
            "nexusflow_events_processed_total 995")
        == std::string::npos) {
        std::cerr << "PROCESSED METRIC: FAIL" << std::endl;
        return 8;
    }

    if (response.find(
            "nexusflow_active_workers 4")
        == std::string::npos) {
        std::cerr << "WORKER METRIC: FAIL" << std::endl;
        return 9;
    }

    if (response.find(
            "nexusflow_queue_depth 7")
        == std::string::npos) {
        std::cerr << "QUEUE METRIC: FAIL" << std::endl;
        return 10;
    }

    std::cout << "HTTP SERVER START: PASS" << std::endl;
    std::cout << "HTTP STATUS: PASS" << std::endl;
    std::cout << "METRICS ENDPOINT: PASS" << std::endl;
    std::cout << "RECEIVED METRIC: PASS" << std::endl;
    std::cout << "PROCESSED METRIC: PASS" << std::endl;
    std::cout << "WORKER METRIC: PASS" << std::endl;
    std::cout << "QUEUE METRIC: PASS" << std::endl;
    std::cout << "METRICS HTTP EXPORTER TEST: PASS" << std::endl;

    return 0;
}
