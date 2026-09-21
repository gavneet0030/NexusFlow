#include "risk_model.hpp"

#include <algorithm>

namespace nexusflow::ml {

ModelPrediction RiskModel::predict(
    const EventFeatures& features) const
{
    double score = 0.0;

    score +=
        std::min(features.amount / 10000.0, 1.0) * 0.35;

    score +=
        std::min(features.priority / 3.0, 1.0) * 0.20;

    score +=
        features.is_fraud_type * 0.30;

    score +=
        features.is_suspicious_type * 0.15;

    score +=
        features.is_chargeback_type * 0.20;

    score +=
        features.high_value * 0.10;

    score +=
        features.critical_priority * 0.10;

    score = std::clamp(score, 0.0, 1.0);

    return ModelPrediction{
        score,
        true
    };
}

} // namespace nexusflow::ml
