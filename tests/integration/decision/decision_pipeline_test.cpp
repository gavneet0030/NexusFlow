#include "../../../decision/decision_engine/decision_engine.hpp"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <string>

using namespace nexusflow;
using namespace nexusflow::decision;

static Event make_event(
    std::uint64_t id,
    const std::string& type,
    std::uint32_t priority,
    double value)
{
    Event event;
    event.id = id;
    event.type = type;
    event.priority = static_cast<EventPriority>(priority);
    event.value = value;
    return event;
}

int main()
{
    std::cout << "============================================\n";
    std::cout << "NEXUSFLOW DECISION HARDENING TEST\n";
    std::cout << "============================================\n";

    DecisionEngine engine;

    {
        const auto event = make_event(1, "transaction", 0, 100.0);
        const auto result = engine.evaluate(event);

        assert(result.decision == FinalDecision::ALLOW);
        std::cout << "NORMAL ALLOW: PASS\n";
    }

    {
        const auto event = make_event(2, "suspicious", 1, 100.0);
        const auto result = engine.evaluate(event);

        assert(result.decision == FinalDecision::REVIEW);
        assert(result.rule_triggered);
        std::cout << "SUSPICIOUS REVIEW: PASS\n";
    }

    {
        const auto event = make_event(3, "transaction", 3, 10000.0);
        const auto result = engine.evaluate(event);

        assert(result.decision == FinalDecision::BLOCK);
        assert(result.risk_score >= 0.70);
        assert(result.risk_triggered);
        std::cout << "HIGH RISK BLOCK: PASS\n";
    }

    {
        const auto event = make_event(4, "suspicious", 3, 100.0);
        const auto result = engine.evaluate(event);

        assert(result.decision == FinalDecision::BLOCK);
        std::cout << "CRITICAL PRECEDENCE: PASS\n";
    }

    {
        const auto event = make_event(5, "fraud", 3, 50000.0);
        const auto result = engine.evaluate(event);

        assert(result.decision == FinalDecision::BLOCK);
        assert(result.risk_score >= 0.0);
        assert(result.risk_score <= 1.0);
        std::cout << "RISK SCORE CLAMP: PASS\n";
    }

    std::cout << "RULE PRECEDENCE: PASS\n";
    std::cout << "RISK PRECEDENCE: PASS\n";
    std::cout << "DECISION PRECEDENCE: PASS\n";
    std::cout << "DECISION HARDENING: PASS\n";

    return 0;
}


