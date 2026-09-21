#include "../../../decision/decision_engine/unified_decision_engine.hpp"

#include <filesystem>
#include <iostream>

int main()
{
    using namespace nexusflow;

    decision::UnifiedDecisionEngine engine;

    const std::filesystem::path model_path =
        std::filesystem::path("ml") /
        "models" /
        "risk_model.json";

    if (!engine.load_ml_model(model_path.string())) {
        std::cerr << "ML MODEL LOAD: FAIL\n";
        return 1;
    }

    std::cout << "ML MODEL LOAD: PASS\n";

    if (!engine.model_loaded()) {
        std::cerr << "MODEL STATE: FAIL\n";
        return 1;
    }

    std::cout << "MODEL STATE: PASS\n";

    // Normal event
    Event normal{};
    normal.value = 100.0;
    normal.type = "normal";

    const auto normal_result =
        engine.evaluate(normal);

    if (normal_result.decision !=
        decision::UnifiedFinalDecision::ALLOW) {
        std::cerr << "NORMAL ALLOW: FAIL\n";
        return 1;
    }

    std::cout << "NORMAL ALLOW: PASS\n";

    // Suspicious event
    Event suspicious{};
    suspicious.value = 15000.0;
    suspicious.type = "suspicious";

    const auto suspicious_result =
        engine.evaluate(suspicious);

    if (suspicious_result.decision ==
        decision::UnifiedFinalDecision::ALLOW) {
        std::cerr << "SUSPICIOUS NON-ALLOW: FAIL\n";
        return 1;
    }

    std::cout << "SUSPICIOUS NON-ALLOW: PASS\n";

    // Fraud event
    Event fraud{};
    fraud.value = 50000.0;
    fraud.type = "fraud";

    const auto fraud_result =
        engine.evaluate(fraud);

    if (fraud_result.decision !=
        decision::UnifiedFinalDecision::BLOCK) {
        std::cerr << "FRAUD BLOCK: FAIL\n";
        return 1;
    }

    std::cout << "FRAUD BLOCK: PASS\n";

    // Score validation
    const auto valid_score =
        [](double value)
        {
            return value >= 0.0 && value <= 1.0;
        };

    if (!valid_score(normal_result.rule_risk_score) ||
        !valid_score(normal_result.risk_engine_score) ||
        !valid_score(normal_result.ml_risk_score) ||
        !valid_score(normal_result.combined_risk_score) ||
        !valid_score(suspicious_result.combined_risk_score) ||
        !valid_score(fraud_result.combined_risk_score)) {
        std::cerr << "SCORE RANGE: FAIL\n";
        return 1;
    }

    std::cout << "SCORE RANGE: PASS\n";

    // Component validation
    if (fraud_result.ml_risk_score <= 0.0) {
        std::cerr << "ML COMPONENT: FAIL\n";
        return 1;
    }

    std::cout << "ML COMPONENT: PASS\n";

    if (fraud_result.risk_engine_score <= 0.0) {
        std::cerr << "RISK COMPONENT: FAIL\n";
        return 1;
    }

    std::cout << "RISK COMPONENT: PASS\n";

    if (fraud_result.reason.empty()) {
        std::cerr << "DECISION REASON: FAIL\n";
        return 1;
    }

    std::cout << "DECISION REASON: PASS\n";

    std::cout
        << "NORMAL COMBINED SCORE: "
        << normal_result.combined_risk_score
        << '\n';

    std::cout
        << "SUSPICIOUS COMBINED SCORE: "
        << suspicious_result.combined_risk_score
        << '\n';

    std::cout
        << "FRAUD COMBINED SCORE: "
        << fraud_result.combined_risk_score
        << '\n';

    std::cout << "UNIFIED DECISION PIPELINE: PASS\n";

    return 0;
}
