#include "tcp_core_pipeline.hpp"

namespace nexusflow::networking {

TcpCorePipeline::TcpCorePipeline(
    std::size_t queue_capacity
)
    : queue_(queue_capacity) {
}

bool TcpCorePipeline::submit_encoded_event(
    const std::string& encoded_event
) {
    EventMessage network_event;

    if (!EventProtocol::decode(
            encoded_event,
            network_event)) {

        rejected_events_.fetch_add(1);
        return false;
    }

    if (network_event.payload.empty()) {
        rejected_events_.fetch_add(1);
        return false;
    }

    nexusflow::Event event;

    event.id = network_event.event_id;
    event.timestamp_ns = network_event.timestamp_ms * 1000000ULL;
    event.type = std::to_string(network_event.type);
    event.source = "tcp";
if (!queue_.try_push(std::move(event))) {
        dropped_events_.fetch_add(1);
        return false;
    }

    accepted_events_.fetch_add(1);

    return true;
}

bool TcpCorePipeline::try_pop_event(
    nexusflow::Event& event
) {
    return queue_.try_pop(event);
}

std::uint64_t TcpCorePipeline::accepted_events() const {
    return accepted_events_.load();
}

std::uint64_t TcpCorePipeline::rejected_events() const {
    return rejected_events_.load();
}

std::uint64_t TcpCorePipeline::dropped_events() const {
    return dropped_events_.load();
}

std::size_t TcpCorePipeline::queue_size() const {
    return queue_.size();
}

}