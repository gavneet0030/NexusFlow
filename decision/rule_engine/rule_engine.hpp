#pragma once

#include "core/event/event.hpp"

namespace nexusflow::decision {

enum class RuleDecision {
    ALLOW,
    REVIEW,
    BLOCK
};

struct RuleResult {
    RuleDecision decision{RuleDecision::ALLOW};
    bool suspicious_type{false};
    bool high_priority{false};
    bool critical{false};
};

class RuleEngine {
public:
    RuleResult evaluate(const Event& event) const;
};

} // namespace nexusflow::decision
