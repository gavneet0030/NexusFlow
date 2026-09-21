#pragma once

#include "event_protocol.hpp"

#include <atomic>
#include <cstdint>
#include <string>

namespace nexusflow::networking {

class TcpPipelineBridge {
public:
    TcpPipelineBridge() = default;

    bool submit_encoded_event(
        const std::string& encoded_event
    );

    std::uint64_t accepted_events() const;
    std::uint64_t rejected_events() const;

private:
    std::atomic<std::uint64_t> accepted_events_{0};
    std::atomic<std::uint64_t> rejected_events_{0};
};

}