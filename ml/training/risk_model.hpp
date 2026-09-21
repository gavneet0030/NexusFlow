#pragma once

#include "../feature_engineering/features.hpp"

namespace nexusflow::ml {

struct ModelPrediction {
    double probability{0.0};
    bool valid{false};
};

class RiskModel {
public:
    ModelPrediction predict(
        const EventFeatures& features) const;
};

} // namespace nexusflow::ml
