#include "tcp_adaptive_pipeline.hpp"
#include "event_protocol.hpp"

#include <chrono>
#include <iostream>
#include <string>
#include <thread>

using nexusflow::networking::EventMessage;
using nexusflow::networking::EventProtocol;
using nexusflow::networking::TcpAdaptivePipeline;
using nexusflow::PipelineMetrics;

int main() {
    TcpAdaptivePipeline pipeline(4096, 4);

    pipeline.start();

    constexpr std::uint64_t event_count = 100;

    for (std::uint64_t i = 0; i < event_count; ++i) {
        EventMessage message;

        message.version = 1;
        message.type = 42;
        message.event_id = 10000 + i;
        message.timestamp_ms = 123456789 + i;
        message.payload = "1.0";

        std::string encoded;

        if (!EventProtocol::encode(
                message,
                encoded)) {

            pipeline.shutdown();

            std::cerr << "EVENT ENCODE: FAIL\n";
            return 1;
        }

        if (!pipeline.submit_encoded_event(encoded)) {
            pipeline.shutdown();

            std::cerr << "TCP EVENT SUBMISSION: FAIL\n";
            return 1;
        }
    }

    std::cout << "TCP EVENT SUBMISSION: PASS\n";

    pipeline.drain();

    for (int i = 0; i < 100; ++i) {
        if (
            pipeline.metrics().processed >=
            event_count
        ) {
            break;
        }

        std::this_thread::sleep_for(
            std::chrono::milliseconds(10)
        );
    }

    PipelineMetrics metrics =
        pipeline.metrics();

    pipeline.shutdown();

    if (pipeline.accepted_events() != event_count) {
        std::cerr << "TCP ACCEPTED COUNT: FAIL\n";
        return 1;
    }

    std::cout << "TCP ACCEPTED COUNT: PASS\n";

    if (metrics.processed != event_count) {
        std::cerr << "CORE PROCESSED COUNT: FAIL\n";
        std::cerr << "Expected: " << event_count << "\n";
        std::cerr << "Actual: " << metrics.processed << "\n";
        return 1;
    }

    std::cout << "CORE PROCESSED COUNT: PASS\n";

    if (metrics.accepted != event_count) {
        std::cerr << "PIPELINE ACCEPTED COUNT: FAIL\n";
        return 1;
    }

    std::cout << "PIPELINE ACCEPTED COUNT: PASS\n";

    if (metrics.rejected != 0) {
        std::cerr << "PIPELINE REJECTED COUNT: FAIL\n";
        return 1;
    }

    std::cout << "PIPELINE REJECTED COUNT: PASS\n";

    std::cout << "TCP ADAPTIVE PIPELINE TEST: PASS\n";

    return 0;
}