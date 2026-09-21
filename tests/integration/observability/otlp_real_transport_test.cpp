#include "otlp_exporter.hpp"

#include <chrono>
#include <cstdint>
#include <iostream>
#include <thread>

int main()
{
    using namespace nexusflow::observability;

    OtlpExporter exporter(
        "http://127.0.0.1:4318"
    );

    if (!exporter.healthy()) {
        std::cerr
            << "EXPORTER HEALTH: FAIL\n";
        return 1;
    }

    const auto now =
        std::chrono::duration_cast<
            std::chrono::nanoseconds>(
                std::chrono::system_clock::now()
                    .time_since_epoch()
            ).count();

    SpanRecord span;

    span.name =
        "nexusflow.otlp.e2e";

    span.context = {
        0x123456789abcdef1ULL,
        0xabcdef1234567891ULL
    };

    span.start_ns =
        static_cast<std::uint64_t>(now);

    std::this_thread::sleep_for(
        std::chrono::milliseconds(2)
    );

    span.end_ns =
        static_cast<std::uint64_t>(
            std::chrono::duration_cast<
                std::chrono::nanoseconds>(
                    std::chrono::system_clock::now()
                        .time_since_epoch()
                ).count()
        );

    span.attributes =
        "component=otlp_e2e_test";

    std::cout
        << "TEST TRACE ID: "
        << std::hex
        << span.context.trace_id
        << std::dec
        << "\n";

    std::cout
        << "TEST SPAN ID: "
        << std::hex
        << span.context.span_id
        << std::dec
        << "\n";

    const bool exported =
        exporter.export_span(span);

    if (!exported) {
        std::cerr
            << "OTLP HTTP EXPORT: FAIL\n";
        return 1;
    }

    std::cout
        << "EXPORTER HEALTH: PASS\n";

    std::cout
        << "SPAN CREATED: PASS\n";

    std::cout
        << "OTLP HTTP EXPORT: PASS\n";

    std::cout
        << "TRACE SENT TO COLLECTOR: PASS\n";

    std::cout
        << "OTLP REAL TRANSPORT TEST: PASS\n";

    return 0;
}
