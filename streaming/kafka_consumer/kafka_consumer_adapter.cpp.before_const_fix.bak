#include "kafka_consumer_adapter.hpp"

#include <librdkafka/rdkafka.h>

#include <cstdint>
#include <memory>
#include <string>
#include <utility>

namespace nexusflow::streaming {

class KafkaConsumerAdapter::Impl {
public:
    rd_kafka_t* producer{nullptr};
    rd_kafka_t* consumer{nullptr};
};

KafkaConsumerAdapter::KafkaConsumerAdapter(
    std::string brokers,
    std::string topic,
    std::string group_id
)
    : impl_(std::make_unique<Impl>()),
      brokers_(std::move(brokers)),
      topic_(std::move(topic)),
      group_id_(std::move(group_id)) {}

KafkaConsumerAdapter::~KafkaConsumerAdapter() {
    disconnect();
}

bool KafkaConsumerAdapter::decode_message(
    const std::string& key,
    const std::string& payload,
    int32_t partition,
    int64_t offset,
    int64_t timestamp_ms,
    KafkaEvent& event
) {
    if (key.empty() || payload.empty()) {
        return false;
    }

    event.key = key;
    event.payload = payload;
    event.partition = partition;
    event.offset = offset;
    event.timestamp_ms = timestamp_ms;

    return true;
}

bool KafkaConsumerAdapter::connect() {

    char error_buffer[512];

    rd_kafka_conf_t* producer_conf = rd_kafka_conf_new();

    if (
        rd_kafka_conf_set(
            producer_conf,
            "bootstrap.servers",
            brokers_.c_str(),
            error_buffer,
            sizeof(error_buffer)
        ) != RD_KAFKA_CONF_OK
    ) {
        rd_kafka_conf_destroy(producer_conf);
        return false;
    }

    impl_->producer = rd_kafka_new(
        RD_KAFKA_PRODUCER,
        producer_conf,
        error_buffer,
        sizeof(error_buffer)
    );

    if (impl_->producer == nullptr) {
        return false;
    }

    rd_kafka_conf_t* consumer_conf = rd_kafka_conf_new();

    if (
        rd_kafka_conf_set(
            consumer_conf,
            "bootstrap.servers",
            brokers_.c_str(),
            error_buffer,
            sizeof(error_buffer)
        ) != RD_KAFKA_CONF_OK
    ) {
        rd_kafka_conf_destroy(consumer_conf);
        return false;
    }

    if (
        rd_kafka_conf_set(
            consumer_conf,
            "group.id",
            group_id_.c_str(),
            error_buffer,
            sizeof(error_buffer)
        ) != RD_KAFKA_CONF_OK
    ) {
        rd_kafka_conf_destroy(consumer_conf);
        return false;
    }

    if (
        rd_kafka_conf_set(
            consumer_conf,
            "auto.offset.reset",
            "earliest",
            error_buffer,
            sizeof(error_buffer)
        ) != RD_KAFKA_CONF_OK
    ) {
        rd_kafka_conf_destroy(consumer_conf);
        return false;
    }

    impl_->consumer = rd_kafka_new(
        RD_KAFKA_CONSUMER,
        consumer_conf,
        error_buffer,
        sizeof(error_buffer)
    );

    if (impl_->consumer == nullptr) {
        return false;
    }

    rd_kafka_poll_set_consumer(impl_->consumer);

    rd_kafka_topic_partition_list_t* topics =
        rd_kafka_topic_partition_list_new(1);

    rd_kafka_topic_partition_list_add(
        topics,
        topic_.c_str(),
        RD_KAFKA_PARTITION_UA
    );

    const rd_kafka_resp_err_t subscribe_error =
        rd_kafka_subscribe(
            impl_->consumer,
            topics
        );

    rd_kafka_topic_partition_list_destroy(topics);

    if (subscribe_error != RD_KAFKA_RESP_ERR_NO_ERROR) {
        return false;
    }

    connected_ = true;

    return true;
}

void KafkaConsumerAdapter::disconnect() {

    if (impl_->consumer != nullptr) {
        rd_kafka_consumer_close(impl_->consumer);
        rd_kafka_destroy(impl_->consumer);
        impl_->consumer = nullptr;
    }

    if (impl_->producer != nullptr) {
        rd_kafka_flush(impl_->producer, 5000);
        rd_kafka_destroy(impl_->producer);
        impl_->producer = nullptr;
    }

    connected_ = false;
}

bool KafkaConsumerAdapter::is_connected() const {
    return connected_;
}

bool KafkaConsumerAdapter::poll(
    KafkaEvent& event,
    int timeout_ms
) {
    if (!connected_ || impl_->consumer == nullptr) {
        return false;
    }

    rd_kafka_message_t* message =
        rd_kafka_consumer_poll(
            impl_->consumer,
            timeout_ms
        );

    if (message == nullptr) {
        return false;
    }

    if (message->err != RD_KAFKA_RESP_ERR_NO_ERROR) {
        rd_kafka_message_destroy(message);
        return false;
    }

    std::string key;

    if (message->key != nullptr && message->key_len > 0) {
        key.assign(
            static_cast<const char*>(message->key),
            message->key_len
        );
    }

    std::string payload;

    if (message->payload != nullptr && message->len > 0) {
        payload.assign(
            static_cast<const char*>(message->payload),
            message->len
        );
    }

    int64_t timestamp_ms = 0;
    rd_kafka_timestamp_type_t timestamp_type;

    const int64_t message_timestamp =
        rd_kafka_message_timestamp(
            message,
            &timestamp_type
        );

    if (message_timestamp >= 0) {
        timestamp_ms = message_timestamp;
    }

    const bool decoded =
        decode_message(
            key,
            payload,
            message->partition,
            message->offset,
            timestamp_ms,
            event
        );

    rd_kafka_message_destroy(message);

    return decoded;
}

bool KafkaConsumerAdapter::produce_test_event(
    const std::string& key,
    const std::string& payload
) {
    if (!produce_event_async(key, payload)) {
        return false;
    }

    return flush_producer(5000);
}

bool KafkaConsumerAdapter::produce_event_async(
    const std::string& key,
    const std::string& payload
) {
    if (!connected_ || impl_->producer == nullptr) {
        return false;
    }

    if (key.empty() || payload.empty()) {
        return false;
    }

    const rd_kafka_resp_err_t result =
        rd_kafka_producev(
            impl_->producer,
            RD_KAFKA_V_TOPIC(topic_.c_str()),
            RD_KAFKA_V_KEY(
                key.data(),
                key.size()
            ),
            RD_KAFKA_V_VALUE(
                payload.data(),
                payload.size()
            ),
            RD_KAFKA_V_END
        );

    return result == RD_KAFKA_RESP_ERR_NO_ERROR;
}

bool KafkaConsumerAdapter::flush_producer(int timeout_ms) {
    if (!connected_ || impl_->producer == nullptr) {
        return false;
    }

    return rd_kafka_flush(impl_->producer, timeout_ms)
        == RD_KAFKA_RESP_ERR_NO_ERROR;
}

const std::string& KafkaConsumerAdapter::brokers() const {
    return brokers_;
}

const std::string& KafkaConsumerAdapter::topic() const {
    return topic_;
}

const std::string& KafkaConsumerAdapter::group_id() const {
    return group_id_;
}

}