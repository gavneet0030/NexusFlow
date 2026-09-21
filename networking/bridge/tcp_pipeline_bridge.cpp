#include "tcp_pipeline_bridge.hpp"

namespace nexusflow::networking {

bool TcpPipelineBridge::submit_encoded_event(
    const std::string& encoded_event
) {
    EventMessage event;

    if (!EventProtocol::decode(encoded_event, event)) {
        rejected_events_.fetch_add(1);
        return false;
    }

    if (event.payload.empty()) {
        rejected_events_.fetch_add(1);
        return false;
    }

    accepted_events_.fetch_add(1);

    return true;
}

std::uint64_t TcpPipelineBridge::accepted_events() const {
    return accepted_events_.load();
}

std::uint64_t TcpPipelineBridge::rejected_events() const {
    return rejected_events_.load();
}

}