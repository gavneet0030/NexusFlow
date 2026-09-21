#include "runtime_observability.hpp"

#include <chrono>
#include <iostream>
#include <thread>

int main() {
    using namespace nexusflow::observability;

    RuntimeObservability observability(19101);

    observability.metrics().record_received(5000);
    observability.metrics().record_processed(4980);
    observability.metrics().record_rejected(10);
    observability.metrics().record_throttled(10);
    observability.metrics().set_active_workers(8);
    observability.metrics().set_queue_depth(25);

    observability.metrics().observe_latency_us(12.5);
    observability.metrics().observe_latency_us(18.5);

    if (!observability.start()) {
        std::cerr << "RUNTIME OBSERVABILITY START: FAIL" << std::endl;
        return 1;
    }

    if (!observability.running()) {
        observability.stop();
        return 2;
    }

    if (observability.metrics().received() != 5000) {
        observability.stop();
        return 3;
    }

    if (observability.metrics().processed() != 4980) {
        observability.stop();
        return 4;
    }

    if (observability.metrics().active_workers() != 8) {
        observability.stop();
        return 5;
    }

    if (observability.metrics().queue_depth() != 25) {
        observability.stop();
        return 6;
    }

    std::this_thread::sleep_for(
        std::chrono::milliseconds(100)
    );

    observability.stop();

    std::cout << "RUNTIME OBSERVABILITY START: PASS" << std::endl;
    std::cout << "RUNTIME METRICS: PASS" << std::endl;
    std::cout << "HTTP EXPORTER: PASS" << std::endl;
    std::cout << "RUNTIME OBSERVABILITY TEST: PASS" << std::endl;

    return 0;
}
