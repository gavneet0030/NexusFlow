#include "nexusflow_grpc_service.hpp"

namespace nexusflow::grpc {

NexusFlowGrpcService::NexusFlowGrpcService(
    std::size_t queue_capacity,
    std::size_t max_workers)
    : pipeline_(queue_capacity, max_workers) {}

NexusFlowGrpcService::~NexusFlowGrpcService() {
    stop();
}

bool NexusFlowGrpcService::start() {
    if (started_) {
        return true;
    }

    pipeline_.start();
    started_ = true;
    return true;
}

void NexusFlowGrpcService::stop() {
    if (!started_) {
        return;
    }

    pipeline_.drain();
    pipeline_.shutdown();
    started_ = false;
}

bool NexusFlowGrpcService::submit_event(
    std::uint64_t event_id,
    std::uint64_t timestamp_ms,
    std::uint32_t type,
    const std::string& payload) {

    if (!started_ || payload.empty()) {
        return false;
    }

    nexusflow::Event event{};

    event.id = event_id;
    event.timestamp_ns = timestamp_ms * 1000000ULL;
    event.type = std::to_string(type);
    event.source = "grpc";

    return pipeline_.submit(std::move(event));
}

bool NexusFlowGrpcService::healthy() const {
    return started_;
}

std::uint64_t NexusFlowGrpcService::processed_events() const {
    return pipeline_.metrics().processed;
}

} // namespace nexusflow::grpc
