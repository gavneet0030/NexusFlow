#pragma once

#include "prometheus_metrics.hpp"

#include <atomic>
#include <cstdint>
#include <string>
#include <thread>

namespace nexusflow::observability {

class MetricsHttpServer {
public:
    MetricsHttpServer(
        PrometheusMetrics& metrics,
        std::uint16_t port = 9100
    );

    ~MetricsHttpServer();

    MetricsHttpServer(const MetricsHttpServer&) = delete;
    MetricsHttpServer& operator=(const MetricsHttpServer&) = delete;

    bool start();
    void stop();

    bool running() const;
    std::uint16_t port() const;

private:
    void run();

    PrometheusMetrics& metrics_;
    std::uint16_t port_;

    std::atomic<bool> running_{false};
    std::thread thread_;
    std::uintptr_t socket_handle_{0};
};

}
