#include "rule_engine.hpp"
#include <cstdint>

namespace nexusflow::decision {

RuleResult RuleEngine::evaluate(const Event& event) const
{
    RuleResult result;

    result.high_priority = static_cast<std::uint32_t>(event.priority) >= 2;
    result.critical = static_cast<std::uint32_t>(event.priority) >= 3;

    result.suspicious_type =
        event.type == "fraud" ||
        event.type == "suspicious" ||
        event.type == "chargeback";

    // Hard rule precedence:
    // CRITICAL -> BLOCK
    // SUSPICIOUS -> REVIEW
    // otherwise -> ALLOW
    //
    // A lower-severity rule must never downgrade BLOCK.
    if (result.critical) {
        result.decision = RuleDecision::BLOCK;
    } else if (result.suspicious_type || result.high_priority) {
        result.decision = RuleDecision::REVIEW;
    } else {
        result.decision = RuleDecision::ALLOW;
    }

    return result;
}

} // namespace nexusflow::decision


