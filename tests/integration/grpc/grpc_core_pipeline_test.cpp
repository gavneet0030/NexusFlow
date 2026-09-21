#include <iostream>
#include <string>

#include "api/grpc/nexusflow_grpc_service.hpp"

int main() {
    nexusflow::grpc::NexusFlowGrpcService service(4096, 8);

    if (!service.start()) {
        std::cerr << "GRPC SERVICE START: FAIL\n";
        return 1;
    }

    constexpr std::uint64_t event_count = 1000;

    for (std::uint64_t i = 0; i < event_count; ++i) {
        if (!service.submit_event(
                i + 1,
                1700000000000ULL + i,
                1,
                "grpc-event")) {
            std::cerr << "GRPC EVENT SUBMISSION: FAIL\n";
            service.stop();
            return 1;
        }
    }

    service.stop();

    const auto processed = service.processed_events();

    std::cout << "GRPC SERVICE START: PASS\n";
    std::cout << "EVENTS SUBMITTED: " << event_count << "\n";
    std::cout << "EVENTS PROCESSED: " << processed << "\n";

    if (processed != event_count) {
        std::cerr << "GRPC CORE PIPELINE TEST: FAIL\n";
        return 1;
    }

    std::cout << "GRPC CORE PIPELINE TEST: PASS\n";
    return 0;
}
