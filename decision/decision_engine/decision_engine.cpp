#include "decision_engine.hpp"

namespace nexusflow::decision {

DecisionResult DecisionEngine::evaluate(const Event& event) const
{
    const RuleResult rules = rule_engine_.evaluate(event);
    const RiskResult risk = risk_engine_.evaluate(event);

    DecisionResult result;
    result.risk_score = risk.score;

    result.rule_triggered =
        rules.decision != RuleDecision::ALLOW;

    result.risk_triggered =
        risk.score >= 0.70;

    // Final precedence:
    // 1. Critical/block rule
    // 2. High risk
    // 3. Review rule
    // 4. Allow
    if (rules.decision == RuleDecision::BLOCK) {
        result.decision = FinalDecision::BLOCK;
    } else if (risk.score >= 0.70) {
        result.decision = FinalDecision::BLOCK;
    } else if (rules.decision == RuleDecision::REVIEW) {
        result.decision = FinalDecision::REVIEW;
    } else {
        result.decision = FinalDecision::ALLOW;
    }

    return result;
}

} // namespace nexusflow::decision
