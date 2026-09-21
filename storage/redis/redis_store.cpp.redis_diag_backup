#include "redis_store.hpp"

#include <memory>
#include <string>

#include <sw/redis++/redis++.h>

namespace nexusflow {

struct RedisStore::Impl {
    std::unique_ptr<sw::redis::Redis> redis;
    bool connected = false;
};

RedisStore::RedisStore()
    : impl_(new Impl()) {
}

RedisStore::~RedisStore() {
    disconnect();
    delete impl_;
}

bool RedisStore::connect(
    const std::string& host,
    std::uint16_t port
) {
    try {

        sw::redis::ConnectionOptions options;

        options.host = host;
        options.port = port;
        options.socket_timeout = std::chrono::milliseconds(2000);
        options.connect_timeout = std::chrono::milliseconds(2000);

        impl_->redis =
            std::make_unique<sw::redis::Redis>(options);

        impl_->redis->ping();

        impl_->connected = true;

        return true;
    }
    catch (...) {

        impl_->redis.reset();
        impl_->connected = false;

        return false;
    }
}

void RedisStore::disconnect() {

    impl_->redis.reset();
    impl_->connected = false;
}

bool RedisStore::is_connected() const {
    return impl_->connected && impl_->redis != nullptr;
}

bool RedisStore::set(
    const std::string& key,
    const std::string& value
) {
    if (!is_connected()) {
        return false;
    }

    try {

        impl_->redis->set(key, value);

        return true;
    }
    catch (...) {

        return false;
    }
}

bool RedisStore::get(
    const std::string& key,
    std::string& value
) {
    if (!is_connected()) {
        return false;
    }

    try {

        auto result = impl_->redis->get(key);

        if (!result) {
            return false;
        }

        value = *result;

        return true;
    }
    catch (...) {

        return false;
    }
}

bool RedisStore::del(
    const std::string& key
) {
    if (!is_connected()) {
        return false;
    }

    try {

        impl_->redis->del(key);

        return true;
    }
    catch (...) {

        return false;
    }
}

bool RedisStore::increment(
    const std::string& key,
    std::int64_t amount
) {
    if (!is_connected()) {
        return false;
    }

    try {

        impl_->redis->incrby(key, amount);

        return true;
    }
    catch (...) {

        return false;
    }
}

}