#include "unified_decision_engine.hpp"

#include <algorithm>

namespace nexusflow::decision {

bool UnifiedDecisionEngine::load_ml_model(
    const std::string& model_path)
{
    return ml_engine_.load_model(model_path);
}

bool UnifiedDecisionEngine::model_loaded() const noexcept
{
    return ml_engine_.model_loaded();
}

UnifiedDecisionResult UnifiedDecisionEngine::evaluate(
    const nexusflow::Event& event) const
{
    UnifiedDecisionResult result;

    const RuleResult rules =
        rule_engine_.evaluate(event);

    const RiskResult risk =
        risk_engine_.evaluate(event);

    const MLDecisionResult ml =
        ml_engine_.evaluate(event);

    result.rule_risk_score =
        rules.decision == RuleDecision::BLOCK
            ? 1.0
            : rules.decision == RuleDecision::REVIEW
                ? 0.5
                : 0.0;

    result.risk_engine_score =
        std::clamp(risk.score, 0.0, 1.0);

    result.ml_risk_score =
        std::clamp(ml.risk_score, 0.0, 1.0);

    result.combined_risk_score =
        std::clamp(
            (result.risk_engine_score +
             result.ml_risk_score) / 2.0,
            0.0,
            1.0);

    result.rule_triggered =
        rules.decision != RuleDecision::ALLOW;

    result.risk_triggered =
        risk.score >= 0.70;

    result.ml_triggered =
        ml.blocked || ml.reviewed;

    // --------------------------------------------------------
    // Final precedence:
    //
    // 1. Blocking rule
    // 2. ML BLOCK
    // 3. Combined high risk
    // 4. Rule REVIEW
    // 5. ML REVIEW
    // 6. Risk REVIEW
    // 7. ALLOW
    // --------------------------------------------------------

    if (rules.decision == RuleDecision::BLOCK) {
        result.decision = UnifiedFinalDecision::BLOCK;
        result.reason = "Blocking rule triggered";
    }
    else if (ml.blocked) {
        result.decision = UnifiedFinalDecision::BLOCK;
        result.reason =
            "ML model exceeded blocking threshold";
    }
    else if (result.combined_risk_score >= 0.70) {
        result.decision = UnifiedFinalDecision::BLOCK;
        result.reason =
            "Combined risk exceeded blocking threshold";
    }
    else if (rules.decision == RuleDecision::REVIEW) {
        result.decision = UnifiedFinalDecision::REVIEW;
        result.reason =
            "Rule engine requires review";
    }
    else if (ml.reviewed) {
        result.decision = UnifiedFinalDecision::REVIEW;
        result.reason =
            "ML model requires review";
    }
    else if (risk.score >= 0.40) {
        result.decision = UnifiedFinalDecision::REVIEW;
        result.reason =
            "Risk engine score requires review";
    }
    else {
        result.decision = UnifiedFinalDecision::ALLOW;
        result.reason =
            "Rules, risk and ML checks passed";
    }

    return result;
}

} // namespace nexusflow::decision
