#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace nexusflow {

struct KafkaMessage {
    std::string payload;
    std::string topic;
    std::int32_t partition{0};
    std::int64_t offset{-1};
};

class KafkaProducer {
public:
    KafkaProducer(
        const std::string& brokers,
        const std::string& topic
    );

    ~KafkaProducer();

    KafkaProducer(const KafkaProducer&) = delete;
    KafkaProducer& operator=(const KafkaProducer&) = delete;

    bool produce(
        const std::string& payload,
        std::int32_t partition = -1
    );

    bool flush(int timeout_ms = 10000);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

class KafkaConsumer {
public:
    KafkaConsumer(
        const std::string& brokers,
        const std::string& group_id,
        const std::string& topic
    );

    ~KafkaConsumer();

    KafkaConsumer(const KafkaConsumer&) = delete;
    KafkaConsumer& operator=(const KafkaConsumer&) = delete;

    bool subscribe();

    bool poll(
        KafkaMessage& message,
        int timeout_ms = 1000
    );

    bool commit();

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

}
