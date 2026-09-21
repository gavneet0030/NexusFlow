#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <string>
#include <thread>
#include "../bridge/tcp_adaptive_pipeline.hpp"
#include "../bridge/tcp_adaptive_pipeline.hpp"

namespace nexusflow::networking {

class TcpEventServer {
public:
    explicit TcpEventServer(std::uint16_t port);
explicit TcpEventServer(
    std::uint16_t port,
    std::size_t queue_capacity,
    std::size_t max_workers);
    ~TcpEventServer();

    TcpEventServer(const TcpEventServer&) = delete;
    TcpEventServer& operator=(const TcpEventServer&) = delete;

    bool start();
    void stop();

    bool is_running() const;

    std::uint64_t received_events() const;
    std::uint64_t rejected_events() const;
    std::uint64_t processed_events() const;

private:
    void run();

    std::uint16_t port_;
    std::atomic<bool> running_{false};
    std::atomic<std::uint64_t> received_events_{0};
    std::atomic<std::uint64_t> rejected_events_{0};
    TcpAdaptivePipeline pipeline_;

    std::thread worker_;
    std::uintptr_t server_socket_{0};
};

}