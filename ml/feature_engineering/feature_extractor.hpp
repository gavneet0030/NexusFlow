#pragma once

#include "core/event/event.hpp"
#include "features.hpp"

namespace nexusflow::ml {

class FeatureExtractor {
public:
    EventFeatures extract(const Event& event) const;
};

} // namespace nexusflow::ml
