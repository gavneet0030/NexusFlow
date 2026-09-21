#include "trace_context.hpp"

#include <atomic>
#include <chrono>
#include <utility>

namespace nexusflow::observability {

namespace {

std::uint64_t next_id()
{
    static std::atomic<std::uint64_t> counter{
        0x9E3779B97F4A7C15ULL
    };

    const auto now =
        static_cast<std::uint64_t>(
            std::chrono::high_resolution_clock::now()
                .time_since_epoch()
                .count()
        );

    return counter.fetch_add(
        now | 1ULL,
        std::memory_order_relaxed
    );
}

}

TraceSpan::TraceSpan(
    std::string name,
    TraceContext context
)
    : name_(std::move(name)),
      context_(context)
{
}

TraceSpan::~TraceSpan()
{
    end();
}

void TraceSpan::set_attribute(
    const std::string&,
    const std::string&
)
{
}

void TraceSpan::set_attribute(
    const std::string&,
    std::uint64_t
)
{
}

void TraceSpan::end()
{
    ended_ = true;
}

bool TraceSpan::ended() const noexcept
{
    return ended_;
}

const TraceContext& TraceSpan::context() const noexcept
{
    return context_;
}

TraceContext Tracer::create_context()
{
    return {
        next_id(),
        next_id()
    };
}

TraceSpan Tracer::start_span(
    const std::string& name,
    const TraceContext& parent
)
{
    TraceContext context;

    if (parent.trace_id != 0) {
        context.trace_id = parent.trace_id;
    } else {
        context.trace_id = next_id();
    }

    context.span_id = next_id();

    return TraceSpan{name, context};
}

}
