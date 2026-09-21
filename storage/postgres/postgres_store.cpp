#include "storage/postgres/postgres_store.hpp"

#include <libpq-fe.h>

#include <cstdlib>
#include <iostream>
#include <string>

namespace nexusflow::storage {

struct PostgresStore::Impl {
    PGconn* connection = nullptr;
    std::uint64_t inserted = 0;

    std::string host;
    int port;
    std::string database;
    std::string user;
    std::string password;

    Impl(
        std::string h,
        int p,
        std::string db,
        std::string u,
        std::string pw
    )
        : host(std::move(h)),
          port(p),
          database(std::move(db)),
          user(std::move(u)),
          password(std::move(pw)) {
    }
};

PostgresStore::PostgresStore(
    const std::string& host,
    int port,
    const std::string& database,
    const std::string& user,
    const std::string& password
)
    : impl_(
        std::make_unique<Impl>(
            host,
            port,
            database,
            user,
            password
        )
    ) {
}

PostgresStore::~PostgresStore() {
    disconnect();
}

bool PostgresStore::connect() {
    if (impl_->connection != nullptr) {
        return connected();
    }

    const std::string port_string =
        std::to_string(impl_->port);

    const char* keywords[] = {
        "host",
        "port",
        "dbname",
        "user",
        "password",
        nullptr
    };

    const char* values[] = {
        impl_->host.c_str(),
        port_string.c_str(),
        impl_->database.c_str(),
        impl_->user.c_str(),
        impl_->password.c_str(),
        nullptr
    };

    impl_->connection =
        PQconnectdbParams(
            keywords,
            values,
            0
        );

    if (impl_->connection == nullptr) {
        std::cerr
            << "POSTGRES CONNECTION: FAIL\n";
        return false;
    }

    if (PQstatus(impl_->connection) != CONNECTION_OK) {
        std::cerr
            << "POSTGRES CONNECTION: FAIL\n"
            << PQerrorMessage(impl_->connection);

        PQfinish(impl_->connection);
        impl_->connection = nullptr;

        return false;
    }

    std::cout
        << "POSTGRES CONNECTION: PASS\n";

    return true;
}

void PostgresStore::disconnect() {
    if (impl_->connection != nullptr) {
        PQfinish(impl_->connection);
        impl_->connection = nullptr;
    }
}

bool PostgresStore::connected() const {
    return
        impl_->connection != nullptr &&
        PQstatus(impl_->connection) == CONNECTION_OK;
}

bool PostgresStore::create_schema() {
    if (!connected()) {
        return false;
    }

    const char* sql = R"SQL(
        CREATE TABLE IF NOT EXISTS nexusflow_events (
            event_id BIGINT PRIMARY KEY,
            timestamp_ms BIGINT NOT NULL,
            event_type TEXT NOT NULL,
            payload TEXT NOT NULL
        );
    )SQL";

    PGresult* result =
        PQexec(
            impl_->connection,
            sql
        );

    if (result == nullptr) {
        return false;
    }

    const bool ok =
        PQresultStatus(result) == PGRES_COMMAND_OK;

    if (!ok) {
        std::cerr
            << "POSTGRES SCHEMA ERROR: "
            << PQresultErrorMessage(result);
    }

    PQclear(result);

    return ok;
}

bool PostgresStore::insert_event(
    const nexusflow::Event& event
) {
    if (!connected()) {
        return false;
    }

    const std::string event_id =
        std::to_string(event.id);

    const std::string timestamp =
        std::to_string(event.timestamp_ns);

    const std::string type =
        event.type;

    const std::string payload =
        std::to_string(event.value);

    const char* values[] = {
        event_id.c_str(),
        timestamp.c_str(),
        type.c_str(),
        payload.c_str()
    };

    PGresult* result =
        PQexecParams(
            impl_->connection,
            "INSERT INTO nexusflow_events "
            "(event_id, timestamp_ms, event_type, payload) "
            "VALUES ($1, $2, $3, $4) "
            "ON CONFLICT (event_id) DO NOTHING;",
            4,
            nullptr,
            values,
            nullptr,
            nullptr,
            0
        );

    if (result == nullptr) {
        return false;
    }

    const ExecStatusType status =
        PQresultStatus(result);

    const bool ok =
        status == PGRES_COMMAND_OK;

    if (ok) {
        const char* affected =
            PQcmdTuples(result);

        if (affected != nullptr &&
            std::string(affected) == "1") {
            ++impl_->inserted;
        }
    }
    else {
        std::cerr
            << "POSTGRES INSERT ERROR: "
            << PQresultErrorMessage(result);
    }

    PQclear(result);

    return ok;
}

std::uint64_t PostgresStore::inserted_events() const {
    return impl_->inserted;
}

}

