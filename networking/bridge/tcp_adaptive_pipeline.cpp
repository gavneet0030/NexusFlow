#include "tcp_adaptive_pipeline.hpp"

namespace nexusflow::networking {

TcpAdaptivePipeline::TcpAdaptivePipeline(
    std::size_t queue_capacity,
    std::size_t max_workers
)
    : pipeline_(
        queue_capacity,
        max_workers
    ) {
}

TcpAdaptivePipeline::~TcpAdaptivePipeline() {
    shutdown();
}

void TcpAdaptivePipeline::start() {
    pipeline_.start();
}

void TcpAdaptivePipeline::shutdown() {
    pipeline_.shutdown();
}

bool TcpAdaptivePipeline::submit_encoded_event(
    const std::string& encoded_event
) {
    EventMessage network_event;

    if (!EventProtocol::decode(
            encoded_event,
            network_event)) {

        ++rejected_events_;
        return false;
    }

    if (network_event.payload.empty()) {
        ++rejected_events_;
        return false;
    }

    Event event;

    event.id = network_event.event_id;

    event.timestamp_ns =
        network_event.timestamp_ms * 1000000ULL;

    event.type =
        std::to_string(network_event.type);

    event.source = "tcp";

    bool accepted = pipeline_.submit(
        std::move(event)
    );

    if (!accepted) {
        ++rejected_events_;
        return false;
    }

    ++accepted_events_;

    return true;
}

void TcpAdaptivePipeline::drain() {
    pipeline_.drain();
}

PipelineMetrics TcpAdaptivePipeline::metrics() const {
    return pipeline_.metrics();
}

std::uint64_t TcpAdaptivePipeline::accepted_events() const {
    return accepted_events_;
}

std::uint64_t TcpAdaptivePipeline::rejected_events() const {
    return rejected_events_;
}


bool TcpAdaptivePipeline::submit(const EventMessage& message) {
    nexusflow::Event event{};

    event.id = message.event_id;
    event.timestamp_ns = message.timestamp_ms * 1000000ULL;
    event.type = std::to_string(message.type);
    event.source = "tcp";

    return pipeline_.submit(std::move(event));
}
}