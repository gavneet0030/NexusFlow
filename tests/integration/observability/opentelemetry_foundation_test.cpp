#include "trace_context.hpp"

#include <iostream>

int main()
{
    using namespace nexusflow::observability;

    const auto root = Tracer::create_context();

    if (root.trace_id == 0 || root.span_id == 0) {
        std::cerr << "TRACE CONTEXT CREATION: FAIL\n";
        return 1;
    }

    auto root_span =
        Tracer::start_span(
            "nexusflow.request",
            root
        );

    if (root_span.context().trace_id != root.trace_id) {
        std::cerr << "ROOT TRACE PROPAGATION: FAIL\n";
        return 1;
    }

    auto child_span =
        Tracer::start_span(
            "nexusflow.processing",
            root_span.context()
        );

    if (child_span.context().trace_id != root.trace_id) {
        std::cerr << "CHILD TRACE PROPAGATION: FAIL\n";
        return 1;
    }

    if (child_span.context().span_id ==
        root_span.context().span_id) {
        std::cerr << "SPAN UNIQUENESS: FAIL\n";
        return 1;
    }

    child_span.set_attribute(
        "event_id",
        static_cast<std::uint64_t>(9000001)
    );

    child_span.end();
    root_span.end();

    if (!child_span.ended() ||
        !root_span.ended()) {
        std::cerr << "SPAN LIFECYCLE: FAIL\n";
        return 1;
    }

    std::cout << "TRACE CONTEXT CREATION: PASS\n";
    std::cout << "ROOT TRACE PROPAGATION: PASS\n";
    std::cout << "CHILD TRACE PROPAGATION: PASS\n";
    std::cout << "SPAN UNIQUENESS: PASS\n";
    std::cout << "SPAN LIFECYCLE: PASS\n";
    std::cout << "OPENTELEMETRY FOUNDATION TEST: PASS\n";

    return 0;
}
