#pragma once

#include "event.hpp"
#include "event_protocol.hpp"
#include "bounded_mpmc_queue.hpp"

#include <atomic>
#include <cstdint>
#include <string>

namespace nexusflow::networking {

class TcpCorePipeline {
public:
    explicit TcpCorePipeline(
        std::size_t queue_capacity = 4096
    );

    bool submit_encoded_event(
        const std::string& encoded_event
    );

    bool try_pop_event(
        nexusflow::Event& event
    );

    std::uint64_t accepted_events() const;
    std::uint64_t rejected_events() const;
    std::uint64_t dropped_events() const;

    std::size_t queue_size() const;

private:
    nexusflow::BoundedMPMCQueue<nexusflow::Event> queue_;

    std::atomic<std::uint64_t> accepted_events_{0};
    std::atomic<std::uint64_t> rejected_events_{0};
    std::atomic<std::uint64_t> dropped_events_{0};
};

}