#include "prometheus_metrics.hpp"

#include <cmath>
#include <iostream>
#include <string>

int main() {
    using nexusflow::observability::PrometheusMetrics;

    PrometheusMetrics metrics;

    metrics.record_received(100);
    metrics.record_processed(90);
    metrics.record_rejected(5);
    metrics.record_throttled(5);
    metrics.record_error(2);

    metrics.set_active_workers(4);
    metrics.set_queue_depth(12);

    metrics.observe_latency_us(10.0);
    metrics.observe_latency_us(20.0);
    metrics.observe_latency_us(30.0);

    if (metrics.received() != 100) return 1;
    if (metrics.processed() != 90) return 2;
    if (metrics.rejected() != 5) return 3;
    if (metrics.throttled() != 5) return 4;
    if (metrics.errors() != 2) return 5;
    if (metrics.active_workers() != 4) return 6;
    if (metrics.queue_depth() != 12) return 7;
    if (metrics.latency_count() != 3) return 8;

    if (std::abs(metrics.latency_sum_us() - 60.0) > 0.001) {
        return 9;
    }

    const std::string output = metrics.render();

    if (output.find("nexusflow_events_received_total 100") == std::string::npos) {
        return 10;
    }

    if (output.find("nexusflow_events_processed_total 90") == std::string::npos) {
        return 11;
    }

    if (output.find("nexusflow_active_workers 4") == std::string::npos) {
        return 12;
    }

    if (output.find("nexusflow_queue_depth 12") == std::string::npos) {
        return 13;
    }

    std::cout << "PROMETHEUS METRICS TEST: PASS" << std::endl;
    std::cout << "RECEIVED: " << metrics.received() << std::endl;
    std::cout << "PROCESSED: " << metrics.processed() << std::endl;
    std::cout << "REJECTED: " << metrics.rejected() << std::endl;
    std::cout << "THROTTLED: " << metrics.throttled() << std::endl;
    std::cout << "ACTIVE WORKERS: " << metrics.active_workers() << std::endl;
    std::cout << "QUEUE DEPTH: " << metrics.queue_depth() << std::endl;
    std::cout << "LATENCY COUNT: " << metrics.latency_count() << std::endl;
    std::cout << "LATENCY SUM US: " << metrics.latency_sum_us() << std::endl;

    return 0;
}
