#pragma once

#include "core/event/event.hpp"

#include <cstdint>
#include <vector>

namespace nexusflow {

struct EndToEndLatencyMetrics {

    double queue_wait_p50_us{0.0};
    double queue_wait_p95_us{0.0};
    double queue_wait_p99_us{0.0};
    double queue_wait_p999_us{0.0};
    double queue_wait_max_us{0.0};

    double processing_p50_us{0.0};
    double processing_p95_us{0.0};
    double processing_p99_us{0.0};
    double processing_p999_us{0.0};
    double processing_max_us{0.0};

    double completion_p50_us{0.0};
    double completion_p95_us{0.0};
    double completion_p99_us{0.0};
    double completion_p999_us{0.0};
    double completion_max_us{0.0};

    double end_to_end_p50_us{0.0};
    double end_to_end_p95_us{0.0};
    double end_to_end_p99_us{0.0};
    double end_to_end_p999_us{0.0};
    double end_to_end_max_us{0.0};

    std::size_t samples{0};
};

}
