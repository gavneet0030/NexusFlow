#include "runtime_observability.hpp"

namespace nexusflow::observability {

RuntimeObservability::RuntimeObservability(
    std::uint16_t port
)
    : metrics_(),
      http_server_(metrics_, port) {
}

RuntimeObservability::~RuntimeObservability() {
    stop();
}

bool RuntimeObservability::start() {
    return http_server_.start();
}

void RuntimeObservability::stop() {
    http_server_.stop();
}

PrometheusMetrics& RuntimeObservability::metrics() {
    return metrics_;
}

const PrometheusMetrics& RuntimeObservability::metrics() const {
    return metrics_;
}

bool RuntimeObservability::running() const {
    return http_server_.running();
}

}
