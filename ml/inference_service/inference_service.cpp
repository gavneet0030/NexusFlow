#include "inference_service.hpp"

namespace nexusflow::ml {

ModelPrediction InferenceService::predict(
    const Event& event) const
{
    const EventFeatures features =
        extractor_.extract(event);

    return model_.predict(features);
}

} // namespace nexusflow::ml
