#include <chrono>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include <grpcpp/grpcpp.h>
#include "nexusflow.grpc.pb.h"

int main() {
    const char* configuredTarget = std::getenv("NEXUSFLOW_GRPC_TARGET");
    const std::string target =
        (configuredTarget != nullptr && configuredTarget[0] != '\0')
            ? configuredTarget
            : "nexusflow:8080";

    auto channel = grpc::CreateChannel(
        target,
        grpc::InsecureChannelCredentials()
    );

    auto stub = ::nexusflow::grpc::NexusFlowService::NewStub(channel);

    constexpr int maxAttempts = 30;
    for (int attempt = 1; attempt <= maxAttempts; ++attempt) {
        grpc::ClientContext context;
        context.set_deadline(
            std::chrono::system_clock::now() + std::chrono::seconds(2)
        );

        ::nexusflow::grpc::HealthRequest request;
        ::nexusflow::grpc::HealthResponse response;

        const grpc::Status status = stub->Health(&context, request, &response);

        if (status.ok()) {
            if (!response.healthy()) {
                std::cerr << "Health RPC succeeded but healthy=false; "
                          << "processed_events=" << response.processed_events()
                          << '\n';
                return 1;
            }

            std::cout << "gRPC Health RPC: PASS; healthy=true; processed_events="
                      << response.processed_events() << '\n';
            return 0;
        }

        std::cerr << "Health attempt " << attempt << "/" << maxAttempts
                  << " failed: " << status.error_message() << '\n';

        if (attempt < maxAttempts) {
            std::this_thread::sleep_for(std::chrono::seconds(2));
        }
    }

    std::cerr << "gRPC Health RPC failed after retries; target=" << target << '\n';
    return 2;
}
