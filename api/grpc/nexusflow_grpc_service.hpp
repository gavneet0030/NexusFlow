#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "core/processor/integrated_adaptive_pipeline.hpp"

namespace nexusflow::grpc {

class NexusFlowGrpcService {
public:
    explicit NexusFlowGrpcService(
        std::size_t queue_capacity = 4096,
        std::size_t max_workers = 8);

    ~NexusFlowGrpcService();

    bool start();
    void stop();

    bool submit_event(
        std::uint64_t event_id,
        std::uint64_t timestamp_ms,
        std::uint32_t type,
        const std::string& payload);

    bool healthy() const;

    std::uint64_t processed_events() const;

private:
    nexusflow::IntegratedAdaptivePipeline pipeline_;
    bool started_{false};
};

} // namespace nexusflow::grpc
