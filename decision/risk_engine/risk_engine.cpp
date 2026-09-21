#include "risk_engine.hpp"
#include <cstdint>

#include <algorithm>

namespace nexusflow::decision {

RiskResult RiskEngine::evaluate(const Event& event) const
{
    double score = 0.0;

    if (static_cast<std::uint32_t>(event.priority) >= 2) {
        score += 0.30;
    }

    if (static_cast<std::uint32_t>(event.priority) >= 3) {
        score += 0.25;
    }

    if (event.type == "fraud" ||
        event.type == "suspicious" ||
        event.type == "chargeback") {
        score += 0.35;
    }

    if (event.value >= 10000.0) {
        score += 0.30;
    } else if (event.value >= 5000.0) {
        score += 0.20;
    } else if (event.value >= 1000.0) {
        score += 0.10;
    }

    score = std::clamp(score, 0.0, 1.0);

    return RiskResult{
        score,
        true
    };
}

} // namespace nexusflow::decision


