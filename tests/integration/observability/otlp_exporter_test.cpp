#include "otlp_exporter.hpp"

#include <iostream>

int main()
{
    using namespace nexusflow::observability;

    OtlpExporter exporter(
        "http://127.0.0.1:4318"
    );

    if (!exporter.healthy()) {
        std::cerr << "EXPORTER HEALTH: FAIL\n";
        return 1;
    }

    if (exporter.endpoint() !=
        "http://127.0.0.1:4318") {
        std::cerr << "EXPORTER ENDPOINT: FAIL\n";
        return 1;
    }

    SpanRecord span;

    span.name = "nexusflow.test";

    span.context = {
        1000001,
        2000001
    };

    span.start_ns = 1000000000;
    span.end_ns = 1000005000;

    span.attributes =
        "service.name=NexusFlow";

    if (!exporter.export_span(span)) {
        std::cerr << "SPAN EXPORT VALIDATION: FAIL\n";
        return 1;
    }

    SpanRecord invalid;
    invalid.name = "invalid";

    if (exporter.export_span(invalid)) {
        std::cerr << "INVALID SPAN REJECTION: FAIL\n";
        return 1;
    }

    std::cout << "EXPORTER HEALTH: PASS\n";
    std::cout << "EXPORTER ENDPOINT: PASS\n";
    std::cout << "SPAN VALIDATION: PASS\n";
    std::cout << "INVALID SPAN REJECTION: PASS\n";
    std::cout << "OTLP EXPORTER TEST: PASS\n";

    return 0;
}
