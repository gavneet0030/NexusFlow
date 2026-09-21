#include <chrono>
#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include <grpcpp/grpcpp.h>

#include "api/grpc/generated/nexusflow.grpc.pb.h"
#include "api/grpc/nexusflow_grpc_server.hpp"

int main() {
    using namespace std::chrono_literals;

    constexpr int event_count = 1000;

    const std::string server_address = "0.0.0.0:50051";
    const std::string client_address = "127.0.0.1:50051";

    nexusflow::grpc::NexusFlowGrpcServer server(
        server_address,
        4096,
        8
    );

    if (!server.start()) {
        std::cerr << "GRPC SERVER START: FAIL\n";
        return 1;
    }

    std::cout << "GRPC SERVER START: PASS\n";

    auto channel = grpc::CreateChannel(
        client_address,
        grpc::InsecureChannelCredentials()
    );

    auto stub =
        nexusflow::grpc::NexusFlowService::NewStub(channel);

    int accepted = 0;

    for (std::uint64_t i = 0; i < event_count; ++i) {
        grpc::ClientContext context;

        nexusflow::grpc::EventRequest request;

        request.set_event_id(i + 1);

        request.set_timestamp_ms(
            static_cast<std::uint64_t>(
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()
                ).count()
            )
        );

        request.set_type(1);
        request.set_payload("nexusflow-test-event");

        nexusflow::grpc::EventResponse response;

        const grpc::Status status =
            stub->SubmitEvent(
                &context,
                request,
                &response
            );

        if (!status.ok()) {
            std::cerr
                << "GRPC RPC FAILURE: "
                << status.error_code()
                << " "
                << status.error_message()
                << "\n";

            server.stop();
            return 1;
        }

        if (response.accepted()) {
            ++accepted;
        }
    }

    std::uint64_t processed = 0;
    bool healthy = false;

    for (int i = 0; i < 100; ++i) {
        grpc::ClientContext context;

        nexusflow::grpc::HealthRequest request;
        nexusflow::grpc::HealthResponse response;

        const grpc::Status status =
            stub->Health(
                &context,
                request,
                &response
            );

        if (status.ok()) {
            healthy = response.healthy();
            processed = response.processed_events();

            if (healthy && processed >= event_count) {
                break;
            }
        }

        std::this_thread::sleep_for(20ms);
    }

    const bool pass =
        accepted == event_count &&
        healthy &&
        processed >= event_count;

    std::cout << "EVENTS SENT: " << event_count << "\n";
    std::cout << "EVENTS ACCEPTED: " << accepted << "\n";
    std::cout << "EVENTS PROCESSED: " << processed << "\n";

    if (!pass) {
        std::cout << "GRPC NETWORK E2E: FAIL\n";
        server.stop();
        return 1;
    }

    server.stop();

    std::cout << "GRPC NETWORK E2E: PASS\n";

    return 0;
}