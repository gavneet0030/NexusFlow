#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "core/event/event.hpp"

namespace nexusflow::storage {

class PostgresStore {
public:
    PostgresStore(
        const std::string& host,
        int port,
        const std::string& database,
        const std::string& user,
        const std::string& password
    );

    ~PostgresStore();

    PostgresStore(const PostgresStore&) = delete;
    PostgresStore& operator=(const PostgresStore&) = delete;

    bool connect();
    void disconnect();

    bool connected() const;

    bool create_schema();

    bool insert_event(const nexusflow::Event& event);

    std::uint64_t inserted_events() const;

private:
    struct Impl;

    std::unique_ptr<Impl> impl_;
};

}



