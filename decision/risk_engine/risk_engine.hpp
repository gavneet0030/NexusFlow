#pragma once

#include "core/event/event.hpp"

namespace nexusflow::decision {

struct RiskResult {
    double score{0.0};
    bool valid{true};
};

class RiskEngine {
public:
    RiskResult evaluate(const Event& event) const;
};

} // namespace nexusflow::decision
