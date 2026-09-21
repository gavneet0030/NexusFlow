#include "../../../decision/decision_engine/ml_decision_engine.hpp"

#include <filesystem>
#include <iostream>

int main()
{
    using namespace nexusflow;

    decision::MLDecisionEngine engine;

    const std::filesystem::path model_path =
        std::filesystem::path("ml") /
        "models" /
        "risk_model.json";

    if (!engine.load_model(model_path.string())) {
        std::cerr << "MODEL LOAD: FAIL\n";
        return 1;
    }

    std::cout << "MODEL LOAD: PASS\n";

    if (!engine.model_loaded()) {
        std::cerr << "MODEL STATE: FAIL\n";
        return 1;
    }

    std::cout << "MODEL STATE: PASS\n";

    Event normal{};
    normal.value = 100.0;
    normal.type = "normal";

    const auto normal_result =
        engine.evaluate(normal);

    if (normal_result.risk_score < 0.0 ||
        normal_result.risk_score > 1.0 ||
        normal_result.action.empty()) {
        std::cerr << "NORMAL INFERENCE: FAIL\n";
        return 1;
    }

    std::cout << "NORMAL INFERENCE: PASS\n";

    Event suspicious{};
    suspicious.value = 15000.0;
    suspicious.type = "suspicious";

    const auto suspicious_result =
        engine.evaluate(suspicious);

    if (suspicious_result.risk_score < 0.0 ||
        suspicious_result.risk_score > 1.0 ||
        suspicious_result.action.empty()) {
        std::cerr << "SUSPICIOUS INFERENCE: FAIL\n";
        return 1;
    }

    std::cout << "SUSPICIOUS INFERENCE: PASS\n";

    Event fraud{};
    fraud.value = 50000.0;
    fraud.type = "fraud";

    const auto fraud_result =
        engine.evaluate(fraud);

    if (fraud_result.risk_score < 0.0 ||
        fraud_result.risk_score > 1.0 ||
        fraud_result.action.empty()) {
        std::cerr << "FRAUD INFERENCE: FAIL\n";
        return 1;
    }

    std::cout << "FRAUD INFERENCE: PASS\n";

    std::cout
        << "NORMAL ACTION: "
        << normal_result.action
        << '\n';

    std::cout
        << "SUSPICIOUS ACTION: "
        << suspicious_result.action
        << '\n';

    std::cout
        << "FRAUD ACTION: "
        << fraud_result.action
        << '\n';

    std::cout
        << "NORMAL SCORE: "
        << normal_result.risk_score
        << '\n';

    std::cout
        << "SUSPICIOUS SCORE: "
        << suspicious_result.risk_score
        << '\n';

    std::cout
        << "FRAUD SCORE: "
        << fraud_result.risk_score
        << '\n';

    std::cout << "DECISION ACTIONS: PASS\n";
    std::cout << "ML DECISION INTEGRATION: PASS\n";

    return 0;
}
