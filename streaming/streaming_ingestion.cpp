#include "streaming_ingestion.hpp"

#include <utility>

namespace nexusflow::streaming {

StreamingIngestion::StreamingIngestion(
    std::string brokers,
    std::string topic,
    std::string group_id
)
    : consumer_(
          std::move(brokers),
          std::move(topic),
          std::move(group_id)
      ) {}

bool StreamingIngestion::start() {
    if (!consumer_.connect()) {
        running_ = false;
        return false;
    }

    running_ = true;
    return true;
}

void StreamingIngestion::stop() {
    consumer_.disconnect();
    running_ = false;
}

bool StreamingIngestion::running() const {
    return running_;
}

KafkaConsumerAdapter& StreamingIngestion::consumer() {
    return consumer_;
}

const KafkaConsumerAdapter& StreamingIngestion::consumer() const {
    return consumer_;
}

}