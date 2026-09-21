#pragma once

#include "../../core/event/event.hpp"
#include "../../ml/feature_engineering/feature_extractor.hpp"
#include "../../ml/training/trained_risk_model.hpp"

#include <string>

namespace nexusflow::decision {

struct MLDecisionResult {
    std::string action;
    double risk_score{0.0};
    bool blocked{false};
    bool reviewed{false};
    bool allowed{false};
    std::string reason;
};

class MLDecisionEngine {
public:
    MLDecisionEngine() = default;

    bool load_model(const std::string& model_path);

    MLDecisionResult evaluate(const nexusflow::Event& event) const;

    bool model_loaded() const noexcept;

private:
    nexusflow::ml::FeatureExtractor feature_extractor_;
    nexusflow::ml::TrainedRiskModel model_;
    bool model_loaded_{false};
};

} // namespace nexusflow::decision
