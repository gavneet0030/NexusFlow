#pragma once

#include "core/event/event.hpp"
#include "../feature_engineering/feature_extractor.hpp"
#include "../training/risk_model.hpp"

namespace nexusflow::ml {

class InferenceService {
public:
    ModelPrediction predict(const Event& event) const;

private:
    FeatureExtractor extractor_;
    RiskModel model_;
};

} // namespace nexusflow::ml
