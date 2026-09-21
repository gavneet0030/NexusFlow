#include "event_protocol.hpp"

#include <cstring>

namespace nexusflow::networking {

namespace {

void write_u32(std::string& output, std::uint32_t value) {
    for (int i = 0; i < 4; ++i) {
        output.push_back(
            static_cast<char>((value >> (i * 8)) & 0xFF)
        );
    }
}

void write_u64(std::string& output, std::uint64_t value) {
    for (int i = 0; i < 8; ++i) {
        output.push_back(
            static_cast<char>((value >> (i * 8)) & 0xFF)
        );
    }
}

bool read_u32(
    const std::string& input,
    std::size_t& position,
    std::uint32_t& value
) {
    if (position + 4 > input.size()) {
        return false;
    }

    value = 0;

    for (int i = 0; i < 4; ++i) {
        value |=
            static_cast<std::uint32_t>(
                static_cast<unsigned char>(input[position++])
            ) << (i * 8);
    }

    return true;
}

bool read_u64(
    const std::string& input,
    std::size_t& position,
    std::uint64_t& value
) {
    if (position + 8 > input.size()) {
        return false;
    }

    value = 0;

    for (int i = 0; i < 8; ++i) {
        value |=
            static_cast<std::uint64_t>(
                static_cast<unsigned char>(input[position++])
            ) << (i * 8);
    }

    return true;
}

}

bool EventProtocol::encode(
    const EventMessage& message,
    std::string& output
) {
    if (message.version != kVersion) {
        return false;
    }

    if (message.payload.size() > kMaxPayloadSize) {
        return false;
    }

    output.clear();
    output.reserve(32 + message.payload.size());

    write_u32(output, message.version);
    write_u32(output, message.type);
    write_u64(output, message.event_id);
    write_u64(output, message.timestamp_ms);

    const auto payload_size =
        static_cast<std::uint32_t>(message.payload.size());

    write_u32(output, payload_size);

    output.append(message.payload);

    return true;
}

bool EventProtocol::decode(
    const std::string& input,
    EventMessage& message
) {
    constexpr std::size_t kHeaderSize = 28;

    if (input.size() < kHeaderSize) {
        return false;
    }

    std::size_t position = 0;

    EventMessage decoded;

    if (!read_u32(input, position, decoded.version)) {
        return false;
    }

    if (!read_u32(input, position, decoded.type)) {
        return false;
    }

    if (!read_u64(input, position, decoded.event_id)) {
        return false;
    }

    if (!read_u64(input, position, decoded.timestamp_ms)) {
        return false;
    }

    if (!read_u32(input, position, decoded.payload_size)) {
        return false;
    }

    if (decoded.version != kVersion) {
        return false;
    }

    if (decoded.payload_size > kMaxPayloadSize) {
        return false;
    }

    if (position + decoded.payload_size != input.size()) {
        return false;
    }

    decoded.payload.assign(
        input.data() + position,
        decoded.payload_size
    );

    message = std::move(decoded);

    return true;
}

}
namespace nexusflow::networking {

bool EventProtocol::decode_frame(
    const std::string& input,
    EventMessage& message,
    std::size_t& consumed
) {
    consumed = 0;

    constexpr std::size_t kHeaderSize =
        sizeof(std::uint32_t) +
        sizeof(std::uint32_t) +
        sizeof(std::uint64_t) +
        sizeof(std::uint64_t) +
        sizeof(std::uint32_t);

    if (input.size() < kHeaderSize) {
        return false;
    }

    std::uint32_t payload_size = 0;

    const auto read_u32 =
        [&input](std::size_t offset) -> std::uint32_t {
            return
                static_cast<std::uint32_t>(
                    static_cast<unsigned char>(input[offset])
                ) |
                (static_cast<std::uint32_t>(
                    static_cast<unsigned char>(input[offset + 1])
                ) << 8) |
                (static_cast<std::uint32_t>(
                    static_cast<unsigned char>(input[offset + 2])
                ) << 16) |
                (static_cast<std::uint32_t>(
                    static_cast<unsigned char>(input[offset + 3])
                ) << 24);
        };

    payload_size = read_u32(24);

    if (payload_size > kMaxPayloadSize) {
        return false;
    }

    const std::size_t frame_size =
        kHeaderSize +
        static_cast<std::size_t>(payload_size);

    if (input.size() < frame_size) {
        return false;
    }

    const std::string frame =
        input.substr(0, frame_size);

    if (!decode(frame, message)) {
        return false;
    }

    consumed = frame_size;

    return true;
}

}