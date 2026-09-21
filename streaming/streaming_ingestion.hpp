#pragma once

#include "kafka_consumer/kafka_consumer_adapter.hpp"

#include <string>

namespace nexusflow::streaming {

class StreamingIngestion {
public:
    StreamingIngestion(
        std::string brokers,
        std::string topic,
        std::string group_id
    );

    bool start();
    void stop();

    bool running() const;

    KafkaConsumerAdapter& consumer();
    const KafkaConsumerAdapter& consumer() const;

private:
    KafkaConsumerAdapter consumer_;
    bool running_{false};
};

}