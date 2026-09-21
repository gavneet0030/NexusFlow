#pragma once

#include "core/event/event.hpp"

namespace nexusflow::ml {

struct EventFeatures {
    double amount{0.0};
    double priority{0.0};
    double is_fraud_type{0.0};
    double is_suspicious_type{0.0};
    double is_chargeback_type{0.0};
    double high_value{0.0};
    double critical_priority{0.0};
};

} // namespace nexusflow::ml
