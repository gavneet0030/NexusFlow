#pragma once

#include <cstdint>
#include <string>

namespace nexusflow {

class TcpServer {
public:
    TcpServer();
    ~TcpServer();

    TcpServer(const TcpServer&) = delete;
    TcpServer& operator=(const TcpServer&) = delete;

    bool start(std::uint16_t port);

    void stop();

    bool is_running() const;

    std::uint16_t port() const;

    std::uint64_t connections_accepted() const;

    std::uint64_t messages_received() const;

private:
    struct Impl;
    Impl* impl_;
};

}
