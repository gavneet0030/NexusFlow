#pragma once

#include <cstdint>
#include <string>

namespace nexusflow::observability {

struct TraceContext {
    std::uint64_t trace_id{0};
    std::uint64_t span_id{0};
};

class TraceSpan {
public:
    TraceSpan(std::string name, TraceContext context);
    ~TraceSpan();

    TraceSpan(const TraceSpan&) = delete;
    TraceSpan& operator=(const TraceSpan&) = delete;

    void set_attribute(
        const std::string& key,
        const std::string& value
    );

    void set_attribute(
        const std::string& key,
        std::uint64_t value
    );

    void end();

    bool ended() const noexcept;

    const TraceContext& context() const noexcept;

private:
    std::string name_;
    TraceContext context_;
    bool ended_{false};
};

class Tracer {
public:
    static TraceContext create_context();

    static TraceSpan start_span(
        const std::string& name,
        const TraceContext& parent = {}
    );
};

}
