#pragma once

#include <atomic>
#include <cstdint>
#include <string>

namespace nexusflow::observability {

class PrometheusMetrics {
public:
    PrometheusMetrics() = default;

    void record_received(std::uint64_t count = 1);
    void record_processed(std::uint64_t count = 1);
    void record_rejected(std::uint64_t count = 1);
    void record_throttled(std::uint64_t count = 1);
    void record_error(std::uint64_t count = 1);

    void set_active_workers(std::uint64_t workers);
    void set_queue_depth(std::uint64_t depth);

    void observe_latency_us(double latency_us);

    std::uint64_t received() const;
    std::uint64_t processed() const;
    std::uint64_t rejected() const;
    std::uint64_t throttled() const;
    std::uint64_t errors() const;
    std::uint64_t active_workers() const;
    std::uint64_t queue_depth() const;

    double latency_sum_us() const;
    std::uint64_t latency_count() const;

    std::string render() const;

private:
    std::atomic<std::uint64_t> received_{0};
    std::atomic<std::uint64_t> processed_{0};
    std::atomic<std::uint64_t> rejected_{0};
    std::atomic<std::uint64_t> throttled_{0};
    std::atomic<std::uint64_t> errors_{0};

    std::atomic<std::uint64_t> active_workers_{0};
    std::atomic<std::uint64_t> queue_depth_{0};

    std::atomic<std::uint64_t> latency_count_{0};
    std::atomic<std::uint64_t> latency_sum_bits_{0};
};

}
