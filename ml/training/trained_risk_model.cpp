#include "trained_risk_model.hpp"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <regex>
#include <sstream>

namespace nexusflow::ml {

namespace {

std::vector<double> extract_array(
    const std::string& text,
    const std::string& key)
{
    const std::regex pattern(
        "\"" + key + "\"\\s*:\\s*\\[([^\\]]+)\\]");

    std::smatch match;

    if (!std::regex_search(text, match, pattern)) {
        return {};
    }

    std::vector<double> values;
    std::stringstream ss(match[1].str());
    std::string token;

    while (std::getline(ss, token, ',')) {
        try {
            values.push_back(std::stod(token));
        } catch (...) {
            return {};
        }
    }

    return values;
}

double extract_scalar(
    const std::string& text,
    const std::string& key)
{
    const std::regex pattern(
        "\"" + key + "\"\\s*:\\s*(-?[0-9eE+\\.]+)");

    std::smatch match;

    if (!std::regex_search(text, match, pattern)) {
        return 0.0;
    }

    return std::stod(match[1].str());
}

double sigmoid(double value)
{
    if (value >= 0.0) {
        const double z = std::exp(-value);
        return 1.0 / (1.0 + z);
    }

    const double z = std::exp(value);
    return z / (1.0 + z);
}

} // namespace

double TrainedRiskModel::predict_probability(
    const std::array<double, kFeatureCount>& features) const
{
    double score = intercept;

    for (std::size_t i = 0; i < kFeatureCount; ++i) {
        const double normalized =
            (features[i] - mean[i]) /
            std::max(scale[i], 1e-12);

        score += weights[i] * normalized;
    }

    return sigmoid(score);
}

bool TrainedRiskModel::valid() const noexcept
{
    for (std::size_t i = 0;
         i < kFeatureCount;
         ++i) {

        if (!std::isfinite(mean[i]) ||
            !std::isfinite(scale[i]) ||
            !std::isfinite(weights[i]) ||
            scale[i] <= 0.0) {
            return false;
        }
    }

    return std::isfinite(intercept);
}

bool TrainedRiskModelLoader::load_json(
    const std::string& path,
    TrainedRiskModel& model)
{
    std::ifstream file(path);

    if (!file) {
        return false;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();

    const std::string text = buffer.str();

    const auto mean = extract_array(text, "mean");
    const auto scale = extract_array(text, "scale");
    const auto weights = extract_array(text, "weights");

    if (mean.size() != TrainedRiskModel::kFeatureCount ||
        scale.size() != TrainedRiskModel::kFeatureCount ||
        weights.size() != TrainedRiskModel::kFeatureCount) {
        return false;
    }

    for (std::size_t i = 0;
         i < TrainedRiskModel::kFeatureCount;
         ++i) {
        model.mean[i] = mean[i];
        model.scale[i] = scale[i];
        model.weights[i] = weights[i];
    }

    model.intercept = extract_scalar(text, "intercept");

    return model.valid();
}

} // namespace nexusflow::ml
