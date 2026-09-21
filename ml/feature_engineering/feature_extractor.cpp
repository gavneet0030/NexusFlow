#include "feature_extractor.hpp"

#include <cstdint>

namespace nexusflow::ml {

EventFeatures FeatureExtractor::extract(const Event& event) const
{
    EventFeatures features;

    features.amount = event.value;

    const auto priority =
        static_cast<std::uint32_t>(event.priority);

    features.priority =
        static_cast<double>(priority);

    features.is_fraud_type =
        event.type == "fraud" ? 1.0 : 0.0;

    features.is_suspicious_type =
        event.type == "suspicious" ? 1.0 : 0.0;

    features.is_chargeback_type =
        event.type == "chargeback" ? 1.0 : 0.0;

    features.high_value =
        event.value >= 10000.0 ? 1.0 : 0.0;

    features.critical_priority =
        priority >= 3 ? 1.0 : 0.0;

    return features;
}

} // namespace nexusflow::ml
