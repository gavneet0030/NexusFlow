#pragma once

#include "kafka_consumer_adapter.hpp"

#include "../../core/event/event.hpp"
#include "../../core/queue/bounded_mpmc_queue.hpp"
#include "../../core/workers/adaptive_worker_pool.hpp"

#include <atomic>
#include <cstdint>
#include <thread>

namespace nexusflow::streaming {

class KafkaPipelineBridge {
public:
    KafkaPipelineBridge(
        KafkaConsumerAdapter& consumer,
        nexusflow::BoundedMPMCQueue<nexusflow::Event>& queue
    );

    ~KafkaPipelineBridge();

    KafkaPipelineBridge(const KafkaPipelineBridge&) = delete;
    KafkaPipelineBridge& operator=(const KafkaPipelineBridge&) = delete;

    bool start();
    void stop();

    bool is_running() const;

    std::uint64_t received_count() const;
    std::uint64_t submitted_count() const;
    std::uint64_t rejected_count() const;

private:
    void run();

    KafkaConsumerAdapter& consumer_;
    nexusflow::BoundedMPMCQueue<nexusflow::Event>& queue_;

    std::atomic<bool> running_{false};
    std::atomic<std::uint64_t> received_{0};
    std::atomic<std::uint64_t> submitted_{0};
    std::atomic<std::uint64_t> rejected_{0};

    std::thread thread_;
};

}