#include "networking/tcp/cp_stream_session.hpp"

#include <cstddef>
#include <cstdint>

namespace nexusflow::networking {

TcpStreamSession::TcpStreamSession(
    std::size_t queue_capacity,
    std::size_t max_workers
)
    : pipeline_(queue_capacity, max_workers) {
}

void TcpStreamSession::start() {
    pipeline_.start();
}

void TcpStreamSession::stop() {
    pipeline_.drain();
    pipeline_.shutdown();
}

bool TcpStreamSession::process_bytes(
    const std::uint8_t* data,
    std::size_t size
) {
    if (data == nullptr && size != 0) {
        return false;
    }

    if (size == 0) {
        return true;
    }

    if (buffer_.size() + size > kMaxBufferedBytes) {
        ++rejected_events_;
        buffer_.clear();
        return false;
    }

    buffer_.insert(
        buffer_.end(),
        data,
        data + size
    );

    return process_buffer();
}

bool TcpStreamSession::process_bytes(
    const std::vector<std::uint8_t>& data
) {
    return process_bytes(
        data.data(),
        data.size()
    );
}

bool TcpStreamSession::process_buffer() {
    while (!buffer_.empty()) {

        std::string frame(
            reinterpret_cast<const char*>(buffer_.data()),
            buffer_.size()
        );

        EventMessage message{};
        std::size_t consumed = 0;

        if (!EventProtocol::decode_frame(
                frame,
                message,
                consumed
            )) {

            return true;
        }

        if (consumed == 0 ||
            consumed > buffer_.size()) {
            ++rejected_events_;
            buffer_.clear();
            return false;
        }

        if (!pipeline_.submit(message)) {

            ++rejected_events_;

            buffer_.erase(
                buffer_.begin(),
                buffer_.begin() +
                    static_cast<std::ptrdiff_t>(consumed)
            );

            continue;
        }

        ++received_events_;

        buffer_.erase(
            buffer_.begin(),
            buffer_.begin() +
                static_cast<std::ptrdiff_t>(consumed)
        );
    }

    return true;
}

std::size_t TcpStreamSession::received_events() const noexcept {
    return received_events_;
}

std::size_t TcpStreamSession::rejected_events() const noexcept {
    return rejected_events_;
}

std::size_t TcpStreamSession::buffered_bytes() const noexcept {
    return buffer_.size();
}

std::size_t TcpStreamSession::processed_events() const noexcept {
    return pipeline_.metrics().processed;
}

} // namespace nexusflow::networking
