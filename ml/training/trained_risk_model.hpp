#pragma once

#include <array>
#include <cstddef>
#include <string>
#include <vector>

namespace nexusflow::ml {

struct TrainedRiskModel {
    static constexpr std::size_t kFeatureCount = 7;

    std::array<double, kFeatureCount> mean{};
    std::array<double, kFeatureCount> scale{};
    std::array<double, kFeatureCount> weights{};
    double intercept{0.0};

    double predict_probability(
        const std::array<double, kFeatureCount>& features) const;

    bool valid() const noexcept;
};

class TrainedRiskModelLoader {
public:
    static bool load_json(
        const std::string& path,
        TrainedRiskModel& model);
};

} // namespace nexusflow::ml
