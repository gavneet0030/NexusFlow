#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

namespace nexusflow::networking {

struct EventMessage {
    std::uint32_t version{1};
    std::uint32_t type{1};
    std::uint64_t event_id{0};
    std::uint64_t timestamp_ms{0};
    std::uint32_t payload_size{0};
    std::string payload;
};

class EventProtocol {
public:
    static constexpr std::uint32_t kVersion = 1;
    static constexpr std::uint32_t kMaxPayloadSize = 1024 * 1024;

    static bool encode(
        const EventMessage& message,
        std::string& output
    );

    static bool decode(
        const std::string& input,
        EventMessage& message
    );

    static bool decode_frame(
        const std::string& input,
        EventMessage& message,
        std::size_t& consumed
    );
};

}
