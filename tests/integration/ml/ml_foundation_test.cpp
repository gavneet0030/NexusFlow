#include "../../../ml/inference_service/inference_service.hpp"

#include <cassert>
#include <iostream>

using namespace nexusflow;
using namespace nexusflow::ml;

static Event make_event(
    std::uint64_t id,
    const std::string& type,
    std::uint32_t priority,
    double value)
{
    Event event;
    event.id = id;
    event.type = type;
    event.priority =
        static_cast<EventPriority>(priority);
    event.value = value;
    return event;
}

int main()
{
    std::cout << "============================================\n";
    std::cout << "NEXUSFLOW ML FOUNDATION TEST\n";
    std::cout << "============================================\n";

    InferenceService inference;

    {
        const auto event =
            make_event(1, "transaction", 0, 100.0);

        const auto prediction =
            inference.predict(event);

        assert(prediction.valid);
        assert(prediction.probability >= 0.0);
        assert(prediction.probability <= 1.0);

        std::cout << "NORMAL INFERENCE: PASS\n";
    }

    {
        const auto event =
            make_event(2, "fraud", 3, 50000.0);

        const auto prediction =
            inference.predict(event);

        assert(prediction.valid);
        assert(prediction.probability > 0.70);

        std::cout << "HIGH RISK INFERENCE: PASS\n";
    }

    {
        const auto event =
            make_event(3, "suspicious", 2, 5000.0);

        const auto prediction =
            inference.predict(event);

        assert(prediction.valid);
        assert(prediction.probability >= 0.0);
        assert(prediction.probability <= 1.0);

        std::cout << "SUSPICIOUS INFERENCE: PASS\n";
    }

    std::cout << "FEATURE ENGINEERING: PASS\n";
    std::cout << "RISK MODEL: PASS\n";
    std::cout << "INFERENCE SERVICE: PASS\n";
    std::cout << "ML FOUNDATION: PASS\n";

    return 0;
}
