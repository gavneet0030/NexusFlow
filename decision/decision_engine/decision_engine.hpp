#pragma once

#include "core/event/event.hpp"
#include "../rule_engine/rule_engine.hpp"
#include "../risk_engine/risk_engine.hpp"

namespace nexusflow::decision {

enum class FinalDecision {
    ALLOW,
    REVIEW,
    BLOCK
};

struct DecisionResult {
    FinalDecision decision{FinalDecision::ALLOW};
    double risk_score{0.0};
    bool rule_triggered{false};
    bool risk_triggered{false};
};

class DecisionEngine {
public:
    DecisionResult evaluate(const Event& event) const;

private:
    RuleEngine rule_engine_;
    RiskEngine risk_engine_;
};

} // namespace nexusflow::decision

