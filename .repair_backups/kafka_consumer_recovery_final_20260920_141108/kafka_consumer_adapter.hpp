#pragma once

#include "kafka_event.hpp"

#include <atomic>
#include <cstdint>
#include <memory>
#include <string>

namespace nexusflow::streaming {

class KafkaConsumerAdapter {
public:
    KafkaConsumerAdapter(
        std::string brokers,
        std::string topic,
        std::string group_id
    );

    ~KafkaConsumerAdapter();

    KafkaConsumerAdapter(const KafkaConsumerAdapter&) = delete;
    KafkaConsumerAdapter& operator=(const KafkaConsumerAdapter&) = delete;

    bool connect();
    void disconnect();
    bool is_connected() const;

    bool poll(KafkaEvent& event, int timeout_ms);

    bool produce_test_event(
        const std::string& key,
        const std::string& payload
    );

    bool produce_event_async(
        const std::string& key,
        const std::string& payload
    );

    bool flush_producer(int timeout_ms = 5000);

    static bool decode_message(
        const std::string& key,
        const std::string& payload,
        int32_t partition,
        int64_t offset,
        int64_t timestamp_ms,
        KafkaEvent& event
    );

    const std::string& brokers() const;
    const std::string& topic() const;
    const std::string& group_id() const;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;

    std::string brokers_;
    std::string topic_;
    std::string group_id_;
    bool connected_{false};
};

}
