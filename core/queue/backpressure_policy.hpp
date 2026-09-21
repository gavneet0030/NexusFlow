#pragma once

#include <cstddef>

namespace nexusflow {

enum class BackpressureAction {
    ACCEPT,
    THROTTLE,
    REJECT
};

struct BackpressureDecision {
    BackpressureAction action{BackpressureAction::ACCEPT};
    std::size_t queue_depth{0};
    double utilization{0.0};
};

class BackpressurePolicy {
public:
    BackpressurePolicy(
        std::size_t soft_limit,
        std::size_t hard_limit
    )
        : soft_limit_(soft_limit),
          hard_limit_(hard_limit) {
    }

    BackpressureDecision evaluate(
        std::size_t queue_depth,
        std::size_t capacity
    ) const {
        BackpressureDecision decision;

        decision.queue_depth = queue_depth;

        if (capacity == 0) {
            decision.action = BackpressureAction::REJECT;
            decision.utilization = 1.0;
            return decision;
        }

        decision.utilization =
            static_cast<double>(queue_depth) /
            static_cast<double>(capacity);

        if (queue_depth >= hard_limit_) {
            decision.action = BackpressureAction::REJECT;
        }
        else if (queue_depth >= soft_limit_) {
            decision.action = BackpressureAction::THROTTLE;
        }
        else {
            decision.action = BackpressureAction::ACCEPT;
        }

        return decision;
    }

private:
    std::size_t soft_limit_;
    std::size_t hard_limit_;
};

} // namespace nexusflow
