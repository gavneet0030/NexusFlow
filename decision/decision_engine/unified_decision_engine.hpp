#pragma once

#include "../../core/event/event.hpp"
#include "../rule_engine/rule_engine.hpp"
#include "../risk_engine/risk_engine.hpp"
#include "ml_decision_engine.hpp"

#include <string>

namespace nexusflow::decision {

enum class UnifiedFinalDecision {
    ALLOW,
    REVIEW,
    BLOCK
};

struct UnifiedDecisionResult {
    UnifiedFinalDecision decision{UnifiedFinalDecision::ALLOW};

    double rule_risk_score{0.0};
    double risk_engine_score{0.0};
    double ml_risk_score{0.0};
    double combined_risk_score{0.0};

    bool rule_triggered{false};
    bool risk_triggered{false};
    bool ml_triggered{false};

    std::string reason;
};

class UnifiedDecisionEngine {
public:
    UnifiedDecisionEngine() = default;

    bool load_ml_model(const std::string& model_path);

    UnifiedDecisionResult evaluate(
        const nexusflow::Event& event) const;

    bool model_loaded() const noexcept;

private:
    RuleEngine rule_engine_;
    RiskEngine risk_engine_;
    MLDecisionEngine ml_engine_;
};

} // namespace nexusflow::decision
