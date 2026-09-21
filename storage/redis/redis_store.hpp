#pragma once

#include <cstdint>
#include <string>

namespace nexusflow {

class RedisStore {
public:
    RedisStore();
    ~RedisStore();

    RedisStore(const RedisStore&) = delete;
    RedisStore& operator=(const RedisStore&) = delete;

    bool connect(
        const std::string& host = "127.0.0.1",
        std::uint16_t port = 6379
    );

    void disconnect();

    bool is_connected() const;

    bool set(
        const std::string& key,
        const std::string& value
    );

    bool get(
        const std::string& key,
        std::string& value
    );

    bool del(
        const std::string& key
    );

    bool increment(
        const std::string& key,
        std::int64_t amount = 1
    );

private:
    struct Impl;
    Impl* impl_;
};

}
