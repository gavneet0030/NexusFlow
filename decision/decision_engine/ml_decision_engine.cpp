#include "ml_decision_engine.hpp"

#include <algorithm>
#include <array>

namespace nexusflow::decision {

bool MLDecisionEngine::load_model(
    const std::string& model_path)
{
    model_loaded_ =
        nexusflow::ml::TrainedRiskModelLoader::load_json(
            model_path,
            model_);

    return model_loaded_;
}

bool MLDecisionEngine::model_loaded() const noexcept
{
    return model_loaded_;
}

MLDecisionResult MLDecisionEngine::evaluate(
    const nexusflow::Event& event) const
{
    MLDecisionResult result;

    if (!model_loaded_) {
        result.action = "REVIEW";
        result.reviewed = true;
        result.reason = "ML model unavailable";
        return result;
    }

    const auto features =
        feature_extractor_.extract(event);

    std::array<
        double,
        nexusflow::ml::TrainedRiskModel::kFeatureCount>
        feature_vector{
            features.amount,
            features.priority,
            features.is_fraud_type,
            features.is_suspicious_type,
            features.is_chargeback_type,
            features.high_value,
            features.critical_priority
        };

    result.risk_score =
        model_.predict_probability(feature_vector);

    result.risk_score =
        std::clamp(result.risk_score, 0.0, 1.0);

    if (result.risk_score >= 0.70) {
        result.action = "BLOCK";
        result.blocked = true;
        result.reason =
            "ML risk score above blocking threshold";
    }
    else if (result.risk_score >= 0.40) {
        result.action = "REVIEW";
        result.reviewed = true;
        result.reason =
            "ML risk score requires review";
    }
    else {
        result.action = "ALLOW";
        result.allowed = true;
        result.reason =
            "ML risk score below review threshold";
    }

    return result;
}

} // namespace nexusflow::decision
