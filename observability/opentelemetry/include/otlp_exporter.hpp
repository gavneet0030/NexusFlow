#pragma once

#include "trace_context.hpp"

#include <cstdint>
#include <string>

namespace nexusflow::observability {

struct SpanRecord {
    std::string name;
    TraceContext context;
    TraceContext parent;
    std::uint64_t start_ns{0};
    std::uint64_t end_ns{0};
    std::string attributes;
};

class OtlpExporter {
public:
    explicit OtlpExporter(
        std::string endpoint =
            "http://127.0.0.1:4318"
    );

    bool export_span(const SpanRecord& span);

    bool healthy() const noexcept;

    const std::string& endpoint() const noexcept;

private:
    std::string endpoint_;
    bool healthy_{false};
};

}
