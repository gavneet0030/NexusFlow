#include "event_replay.hpp"

#include <algorithm>
#include <chrono>
#include <fstream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace nexusflow::streaming {

EventReplay::EventReplay()
    : rate_eps_(0) {
}

EventReplay::EventReplay(std::int64_t rate_eps)
    : rate_eps_(rate_eps) {
}

bool EventReplay::valid() const {
    return rate_eps_ > 0 || !events_.empty();
}

std::int64_t EventReplay::rate_eps() const {
    return rate_eps_;
}

std::int64_t EventReplay::interval_microseconds() const {
    if (rate_eps_ <= 0) {
        return 0;
    }

    return 1'000'000 / rate_eps_;
}

bool EventReplay::load_csv(const std::string& path) {
    std::ifstream file(path);

    if (!file.is_open()) {
        return false;
    }

    events_.clear();

    std::string line;

    if (!std::getline(file, line)) {
        return false;
    }

    std::uint64_t id = 1;

    while (std::getline(file, line)) {
        if (line.empty()) {
            continue;
        }

        std::stringstream stream(line);
        std::string field;
        std::vector<std::string> fields;

        while (std::getline(stream, field, ',')) {
            fields.push_back(field);
        }

        nexusflow::Event event;

        event.id = id++;

        event.timestamp_ns =
            static_cast<std::uint64_t>(
                std::chrono::duration_cast<std::chrono::nanoseconds>(
                    std::chrono::system_clock::now().time_since_epoch()
                ).count()
            );

        event.priority = nexusflow::EventPriority::NORMAL;
        event.value = 0.0;
        event.source = "replay";
        event.type = "dataset";

        if (!fields.empty()) {
            try {
                event.value = std::stod(fields.back());
            }
            catch (...) {
                event.value = 0.0;
            }
        }

        events_.push_back(std::move(event));
    }

    return !events_.empty();
}

std::size_t EventReplay::size() const {
    return events_.size();
}

bool EventReplay::empty() const {
    return events_.empty();
}

nexusflow::Event EventReplay::make_event(std::size_t index) const {
    if (index >= events_.size()) {
        return nexusflow::Event{};
    }

    return events_[index];
}

} // namespace nexusflow::streaming
