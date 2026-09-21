#pragma once

#include "core/event/event.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace nexusflow::streaming {

class EventReplay {
public:
    EventReplay();
    explicit EventReplay(std::int64_t rate_eps);

    bool valid() const;

    std::int64_t rate_eps() const;
    std::int64_t interval_microseconds() const;

    bool load_csv(const std::string& path);

    std::size_t size() const;
    bool empty() const;

    nexusflow::Event make_event(std::size_t index) const;

private:
    std::int64_t rate_eps_{0};
    std::vector<nexusflow::Event> events_;
};

}
