#include <iostream>

#include "redis_store.hpp"

int main() {
    nexusflow::RedisStore redis;

    if (!redis.connect()) {
        std::cerr << "Redis connection failed." << std::endl;
        return 1;
    }

    if (!redis.set("nexusflow:test:key", "hello")) {
        std::cerr << "Redis SET failed." << std::endl;
        return 1;
    }

    if (!redis.increment("nexusflow:test:counter", 5)) {
        std::cerr << "Redis INCRBY failed." << std::endl;
        return 1;
    }

    if (!redis.del("nexusflow:test:key")) {
        std::cerr << "Redis DEL failed." << std::endl;
        return 1;
    }

    redis.disconnect();

    std::cout << "REDIS INTEGRATION TEST: PASS" << std::endl;

    return 0;
}
