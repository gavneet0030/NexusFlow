#pragma once

#include "cp_stream_session.hpp"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <thread>

#ifdef _WIN32
#include <winsock2.h>
#endif

namespace nexusflow::networking {

class TcpPersistentEventServer {
public:
    explicit TcpPersistentEventServer(
        std::uint16_t port,
        std::size_t queue_capacity = 4096,
        std::size_t max_workers = 8
    );

    ~TcpPersistentEventServer();

    bool start();
    void stop();

    std::size_t received_events() const noexcept;
    std::size_t rejected_events() const noexcept;
    std::size_t processed_events() const noexcept;

private:
    void run();

    std::uint16_t port_;
    std::size_t queue_capacity_;
    std::size_t max_workers_;

    TcpStreamSession session_;

    std::atomic<bool> running_{false};
    std::thread server_thread_;

#ifdef _WIN32
    SOCKET listen_socket_{INVALID_SOCKET};
#endif
};

} // namespace nexusflow::networking
