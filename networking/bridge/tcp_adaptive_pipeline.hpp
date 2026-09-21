#pragma once

#include "event_protocol.hpp"
#include "../../core/processor/integrated_adaptive_pipeline.hpp"

#include <cstddef>
#include <cstdint>
#include <string>

namespace nexusflow::networking {

class TcpAdaptivePipeline {
public:
    TcpAdaptivePipeline(
        std::size_t queue_capacity,
        std::size_t max_workers
    );

    ~TcpAdaptivePipeline();

    TcpAdaptivePipeline(
        const TcpAdaptivePipeline&
    ) = delete;

    TcpAdaptivePipeline& operator=(
        const TcpAdaptivePipeline&
    ) = delete;

    void start();

    void shutdown();

    bool submit_encoded_event(
        const std::string& encoded_event
    );

    void drain();

    PipelineMetrics metrics() const;

    std::uint64_t accepted_events() const;
    std::uint64_t rejected_events() const;

    bool submit(const EventMessage& message);

private:
    IntegratedAdaptivePipeline pipeline_;
    std::uint64_t accepted_events_{0};
    std::uint64_t rejected_events_{0};
};

}