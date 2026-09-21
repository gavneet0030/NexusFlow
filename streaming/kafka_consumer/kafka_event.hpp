#pragma once

#include <cstdint>
#include <string>

namespace nexusflow::streaming {

struct KafkaEvent {
    std::string key;
    std::string payload;
    std::int64_t timestamp_ms{0};
    std::int32_t partition{-1};
    std::int64_t offset{-1};
};

}