#pragma once

#include "prometheus_metrics.hpp"
#include "metrics_http_server.hpp"

#include <cstdint>

namespace nexusflow::observability {

class RuntimeObservability {
public:
    explicit RuntimeObservability(
        std::uint16_t port = 9100
    );

    ~RuntimeObservability();

    RuntimeObservability(const RuntimeObservability&) = delete;
    RuntimeObservability& operator=(const RuntimeObservability&) = delete;

    bool start();
    void stop();

    PrometheusMetrics& metrics();
    const PrometheusMetrics& metrics() const;

    bool running() const;

private:
    PrometheusMetrics metrics_;
    MetricsHttpServer http_server_;
};

}


