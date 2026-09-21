#include "prometheus_metrics.hpp"

#include <cstring>
#include <iomanip>
#include <sstream>

namespace nexusflow::observability {

namespace {

std::uint64_t double_to_bits(double value) {
    std::uint64_t bits = 0;
    std::memcpy(&bits, &value, sizeof(bits));
    return bits;
}

double bits_to_double(std::uint64_t bits) {
    double value = 0.0;
    std::memcpy(&value, &bits, sizeof(value));
    return value;
}

void atomic_add_double(
    std::atomic<std::uint64_t>& target,
    double value
) {
    auto current = target.load(std::memory_order_relaxed);

    while (true) {
        const double current_value = bits_to_double(current);
        const double next_value = current_value + value;
        const auto next = double_to_bits(next_value);

        if (target.compare_exchange_weak(
                current,
                next,
                std::memory_order_relaxed,
                std::memory_order_relaxed)) {
            return;
        }
    }
}

}

void PrometheusMetrics::record_received(std::uint64_t count) {
    received_.fetch_add(count, std::memory_order_relaxed);
}

void PrometheusMetrics::record_processed(std::uint64_t count) {
    processed_.fetch_add(count, std::memory_order_relaxed);
}

void PrometheusMetrics::record_rejected(std::uint64_t count) {
    rejected_.fetch_add(count, std::memory_order_relaxed);
}

void PrometheusMetrics::record_throttled(std::uint64_t count) {
    throttled_.fetch_add(count, std::memory_order_relaxed);
}

void PrometheusMetrics::record_error(std::uint64_t count) {
    errors_.fetch_add(count, std::memory_order_relaxed);
}

void PrometheusMetrics::set_active_workers(std::uint64_t workers) {
    active_workers_.store(workers, std::memory_order_relaxed);
}

void PrometheusMetrics::set_queue_depth(std::uint64_t depth) {
    queue_depth_.store(depth, std::memory_order_relaxed);
}

void PrometheusMetrics::observe_latency_us(double latency_us) {
    if (latency_us < 0.0) {
        return;
    }

    latency_count_.fetch_add(1, std::memory_order_relaxed);
    atomic_add_double(latency_sum_bits_, latency_us);
}

std::uint64_t PrometheusMetrics::received() const {
    return received_.load(std::memory_order_relaxed);
}

std::uint64_t PrometheusMetrics::processed() const {
    return processed_.load(std::memory_order_relaxed);
}

std::uint64_t PrometheusMetrics::rejected() const {
    return rejected_.load(std::memory_order_relaxed);
}

std::uint64_t PrometheusMetrics::throttled() const {
    return throttled_.load(std::memory_order_relaxed);
}

std::uint64_t PrometheusMetrics::errors() const {
    return errors_.load(std::memory_order_relaxed);
}

std::uint64_t PrometheusMetrics::active_workers() const {
    return active_workers_.load(std::memory_order_relaxed);
}

std::uint64_t PrometheusMetrics::queue_depth() const {
    return queue_depth_.load(std::memory_order_relaxed);
}

double PrometheusMetrics::latency_sum_us() const {
    return bits_to_double(
        latency_sum_bits_.load(std::memory_order_relaxed)
    );
}

std::uint64_t PrometheusMetrics::latency_count() const {
    return latency_count_.load(std::memory_order_relaxed);
}

std::string PrometheusMetrics::render() const {
    std::ostringstream out;

    out << std::fixed << std::setprecision(3);

    out << "# HELP nexusflow_events_received_total Total events received.\n";
    out << "# TYPE nexusflow_events_received_total counter\n";
    out << "nexusflow_events_received_total "
        << received() << "\n\n";

    out << "# HELP nexusflow_events_processed_total Total events processed.\n";
    out << "# TYPE nexusflow_events_processed_total counter\n";
    out << "nexusflow_events_processed_total "
        << processed() << "\n\n";

    out << "# HELP nexusflow_events_rejected_total Total events rejected.\n";
    out << "# TYPE nexusflow_events_rejected_total counter\n";
    out << "nexusflow_events_rejected_total "
        << rejected() << "\n\n";

    out << "# HELP nexusflow_events_throttled_total Total throttled events.\n";
    out << "# TYPE nexusflow_events_throttled_total counter\n";
    out << "nexusflow_events_throttled_total "
        << throttled() << "\n\n";

    out << "# HELP nexusflow_processing_errors_total Total processing errors.\n";
    out << "# TYPE nexusflow_processing_errors_total counter\n";
    out << "nexusflow_processing_errors_total "
        << errors() << "\n\n";

    out << "# HELP nexusflow_active_workers Current active worker count.\n";
    out << "# TYPE nexusflow_active_workers gauge\n";
    out << "nexusflow_active_workers "
        << active_workers() << "\n\n";

    out << "# HELP nexusflow_queue_depth Current queue depth.\n";
    out << "# TYPE nexusflow_queue_depth gauge\n";
    out << "nexusflow_queue_depth "
        << queue_depth() << "\n\n";

    out << "# HELP nexusflow_processing_latency_us_sum Total observed processing latency in microseconds.\n";
    out << "# TYPE nexusflow_processing_latency_us_sum counter\n";
    out << "nexusflow_processing_latency_us_sum "
        << latency_sum_us() << "\n\n";

    out << "# HELP nexusflow_processing_latency_us_count Number of observed processing latencies.\n";
    out << "# TYPE nexusflow_processing_latency_us_count counter\n";
    out << "nexusflow_processing_latency_us_count "
        << latency_count() << "\n";

    return out.str();
}

}
