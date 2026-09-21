#include "trained_risk_model.hpp"

#include <array>
#include <chrono>
#include <filesystem>
#include <iomanip>
#include <iostream>

int main()
{
    using namespace nexusflow::ml;

    const std::filesystem::path model_path =
        std::filesystem::path("ml") /
        "models" /
        "risk_model.json";

    TrainedRiskModel model;

    if (!TrainedRiskModelLoader::load_json(
            model_path.string(),
            model)) {
        std::cerr << "MODEL LOAD: FAIL\n";
        return 1;
    }

    if (!model.valid()) {
        std::cerr << "MODEL VALIDATION: FAIL\n";
        return 1;
    }

    const std::array<double, TrainedRiskModel::kFeatureCount>
        features{
            15000.0,
            3.0,
            0.0,
            1.0,
            0.0,
            1.0,
            1.0
        };

    const double probability =
        model.predict_probability(features);

    if (!(probability >= 0.0 && probability <= 1.0)) {
        std::cerr << "PROBABILITY RANGE: FAIL\n";
        return 1;
    }

    constexpr std::size_t iterations = 1'000'000;

    volatile double sink = 0.0;

    const auto start =
        std::chrono::steady_clock::now();

    for (std::size_t i = 0; i < iterations; ++i) {
        sink += model.predict_probability(features);
    }

    const auto end =
        std::chrono::steady_clock::now();

    const double seconds =
        std::chrono::duration<double>(
            end - start
        ).count();

    if (seconds <= 0.0) {
        std::cerr << "BENCHMARK TIMING: FAIL\n";
        return 1;
    }

    const double throughput =
        static_cast<double>(iterations) / seconds;

    const double latency_us =
        seconds * 1'000'000.0 /
        static_cast<double>(iterations);

    std::cout << std::fixed << std::setprecision(6);

    std::cout << "MODEL LOAD: PASS\n";
    std::cout << "MODEL VALIDATION: PASS\n";
    std::cout << "SAMPLE RISK PROBABILITY: "
              << probability << '\n';
    std::cout << "INFERENCE ITERATIONS: "
              << iterations << '\n';
    std::cout << "INFERENCE THROUGHPUT EPS: "
              << throughput << '\n';
    std::cout << "INFERENCE LATENCY US: "
              << latency_us << '\n';
    std::cout << "C++ INFERENCE: PASS\n";
    std::cout << "INFERENCE BENCHMARK: PASS\n";

    return sink >= 0.0 ? 0 : 1;
}
