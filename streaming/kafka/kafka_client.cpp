#include "kafka_client.hpp"

#include <rdkafkacpp.h>

#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace nexusflow {

class KafkaProducer::Impl {
public:
    Impl(
        const std::string& brokers,
        const std::string& topic_name
    )
        : topic(topic_name) {

        std::string error;

        RdKafka::Conf* global_config =
            RdKafka::Conf::create(
                RdKafka::Conf::CONF_GLOBAL
            );

        if (!global_config) {
            throw std::runtime_error(
                "Failed to create Kafka producer configuration"
            );
        }

        if (
            global_config->set(
                "bootstrap.servers",
                brokers,
                error
            ) != RdKafka::Conf::CONF_OK
        ) {
            delete global_config;

            throw std::runtime_error(
                "Failed to configure bootstrap.servers: " + error
            );
        }

        if (
            global_config->set(
                "acks",
                "all",
                error
            ) != RdKafka::Conf::CONF_OK
        ) {
            delete global_config;

            throw std::runtime_error(
                "Failed to configure producer acks: " + error
            );
        }

        if (
            global_config->set(
                "enable.idempotence",
                "true",
                error
            ) != RdKafka::Conf::CONF_OK
        ) {
            delete global_config;

            throw std::runtime_error(
                "Failed to configure producer idempotence: " + error
            );
        }

        if (
            global_config->set(
                "message.timeout.ms",
                "10000",
                error
            ) != RdKafka::Conf::CONF_OK
        ) {
            delete global_config;

            throw std::runtime_error(
                "Failed to configure message timeout: " + error
            );
        }

        RdKafka::Producer* producer_raw =
            RdKafka::Producer::create(
                global_config,
                error
            );

        delete global_config;

        if (!producer_raw) {
            throw std::runtime_error(
                "Failed to create Kafka producer: " + error
            );
        }

        producer.reset(producer_raw);
    }

    std::unique_ptr<RdKafka::Producer> producer;
    std::string topic;
};

KafkaProducer::KafkaProducer(
    const std::string& brokers,
    const std::string& topic
)
    : impl_(
        std::make_unique<Impl>(
            brokers,
            topic
        )
    ) {
}

KafkaProducer::~KafkaProducer() = default;

bool KafkaProducer::produce(
    const std::string& payload,
    std::int32_t partition
) {

    const std::int32_t target_partition =
        partition >= 0
            ? partition
            : RdKafka::Topic::PARTITION_UA;

    const RdKafka::ErrorCode result =
        impl_->producer->produce(
            impl_->topic,
            target_partition,
            RdKafka::Producer::RK_MSG_COPY,
            const_cast<char*>(payload.data()),
            payload.size(),
            nullptr,
            0,
            0,
            nullptr
        );

    if (result != RdKafka::ERR_NO_ERROR) {
        return false;
    }

    impl_->producer->poll(0);

    return true;
}

bool KafkaProducer::flush(int timeout_ms) {

    impl_->producer->flush(timeout_ms);

    return impl_->producer->outq_len() == 0;
}

class KafkaConsumer::Impl {
public:
    Impl(
        const std::string& brokers,
        const std::string& group_id,
        const std::string& topic_name
    )
        : topic(topic_name) {

        std::string error;

        RdKafka::Conf* config =
            RdKafka::Conf::create(
                RdKafka::Conf::CONF_GLOBAL
            );

        if (!config) {
            throw std::runtime_error(
                "Failed to create Kafka consumer configuration"
            );
        }

        if (
            config->set(
                "bootstrap.servers",
                brokers,
                error
            ) != RdKafka::Conf::CONF_OK
        ) {
            delete config;

            throw std::runtime_error(
                "Failed to configure consumer brokers: " + error
            );
        }

        if (
            config->set(
                "group.id",
                group_id,
                error
            ) != RdKafka::Conf::CONF_OK
        ) {
            delete config;

            throw std::runtime_error(
                "Failed to configure consumer group: " + error
            );
        }

        if (
            config->set(
                "auto.offset.reset",
                "earliest",
                error
            ) != RdKafka::Conf::CONF_OK
        ) {
            delete config;

            throw std::runtime_error(
                "Failed to configure offset reset: " + error
            );
        }

        if (
            config->set(
                "enable.auto.commit",
                "false",
                error
            ) != RdKafka::Conf::CONF_OK
        ) {
            delete config;

            throw std::runtime_error(
                "Failed to disable auto commit: " + error
            );
        }

        RdKafka::KafkaConsumer* consumer_raw =
            RdKafka::KafkaConsumer::create(
                config,
                error
            );

        delete config;

        if (!consumer_raw) {
            throw std::runtime_error(
                "Failed to create Kafka consumer: " + error
            );
        }

        consumer.reset(consumer_raw);
    }

    std::unique_ptr<RdKafka::KafkaConsumer> consumer;
    std::string topic;
};

KafkaConsumer::KafkaConsumer(
    const std::string& brokers,
    const std::string& group_id,
    const std::string& topic
)
    : impl_(
        std::make_unique<Impl>(
            brokers,
            group_id,
            topic
        )
    ) {
}

KafkaConsumer::~KafkaConsumer() {

    if (impl_ && impl_->consumer) {
        impl_->consumer->close();
    }
}

bool KafkaConsumer::subscribe() {

    const std::vector<std::string> topics = {
        impl_->topic
    };

    return impl_->consumer->subscribe(topics)
        == RdKafka::ERR_NO_ERROR;
}

bool KafkaConsumer::poll(
    KafkaMessage& message,
    int timeout_ms
) {

    std::unique_ptr<RdKafka::Message> record(
        impl_->consumer->consume(timeout_ms)
    );

    if (!record) {
        return false;
    }

    if (
        record->err() == RdKafka::ERR__TIMED_OUT ||
        record->err() == RdKafka::ERR__PARTITION_EOF
    ) {
        return false;
    }

    if (record->err() != RdKafka::ERR_NO_ERROR) {
        return false;
    }

    if (!record->payload() || record->len() == 0) {
        return false;
    }

    message.payload.assign(
        static_cast<const char*>(record->payload()),
        record->len()
    );

    message.topic = record->topic_name();
    message.partition = record->partition();
    message.offset = record->offset();

    return true;
}

bool KafkaConsumer::commit() {

    return impl_->consumer->commitSync()
        == RdKafka::ERR_NO_ERROR;
}

}
