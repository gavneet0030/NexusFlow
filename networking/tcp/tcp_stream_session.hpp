#pragma once

#include "../protocol/event_protocol.hpp"
#include "../bridge/tcp_adaptive_pipeline.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace nexusflow::networking {

class TcpStreamSession {
public:
    explicit TcpStreamSession(
        std::size_t queue_capacity = 4096,
        std::size_t max_workers = 8
    );

    bool process_bytes(const std::uint8_t* data, std::size_t size);

    bool process_bytes(const std::vector<std::uint8_t>& data);

    std::size_t received_events() const noexcept;
    std::size_t rejected_events() const noexcept;
    std::size_t buffered_bytes() const noexcept;
    std::size_t processed_events() const noexcept;

    void start();
    void stop();

private:
    static constexpr std::size_t kMaxBufferedBytes = 2 * 1024 * 1024;

    std::vector<std::uint8_t> buffer_;
    std::size_t received_events_{0};
    std::size_t rejected_events_{0};

    TcpAdaptivePipeline pipeline_;

    bool process_buffer();
};

} // namespace nexusflow::networking
