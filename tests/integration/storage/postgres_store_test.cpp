#include "storage/postgres/postgres_store.hpp"
#include "core/event/event.hpp"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>

int main() {
    std::cout << "POSTGRES NATIVE INTEGRATION TEST" << std::endl;

    const char* host_env = std::getenv("NEXUSFLOW_POSTGRES_HOST");
    const char* port_env = std::getenv("NEXUSFLOW_POSTGRES_PORT");
    const char* database_env = std::getenv("NEXUSFLOW_POSTGRES_DATABASE");
    const char* user_env = std::getenv("NEXUSFLOW_POSTGRES_USER");
    const char* password_env = std::getenv("NEXUSFLOW_POSTGRES_PASSWORD");

    if (host_env == nullptr ||
        port_env == nullptr ||
        database_env == nullptr ||
        user_env == nullptr ||
        password_env == nullptr ||
        std::string(password_env).empty()) {
        std::cerr
            << "POSTGRES CONFIG: FAIL - required environment variables missing"
            << std::endl;
        return 1;
    }

    int port = 0;

    try {
        port = std::stoi(port_env);
    }
    catch (...) {
        std::cerr
            << "POSTGRES CONFIG: FAIL - invalid NEXUSFLOW_POSTGRES_PORT"
            << std::endl;
        return 1;
    }

    if (port <= 0) {
        std::cerr
            << "POSTGRES CONFIG: FAIL - invalid NEXUSFLOW_POSTGRES_PORT"
            << std::endl;
        return 1;
    }

    nexusflow::storage::PostgresStore store(
        host_env,
        port,
        database_env,
        user_env,
        password_env
    );

    std::cout << "=== CONNECT ===" << std::endl;

    if (!store.connect()) {
        std::cerr << "POSTGRES CONNECTION: FAIL" << std::endl;
        return 1;
    }

    std::cout << "POSTGRES CONNECTION: PASS" << std::endl;

    std::cout << "=== CREATE SCHEMA ===" << std::endl;

    if (!store.create_schema()) {
        std::cerr << "POSTGRES SCHEMA: FAIL" << std::endl;
        return 1;
    }

    std::cout << "POSTGRES SCHEMA: PASS" << std::endl;

    nexusflow::Event event{};

    event.id = 9000001;
    event.timestamp_ns = 1735689600000000000ULL;
    event.priority = nexusflow::EventPriority::HIGH;
    event.value = 123.456;
    event.source = "postgres-integration-test";
    event.type = "transaction";

    std::cout << "=== INSERT EVENT ===" << std::endl;

    if (!store.insert_event(event)) {
        std::cerr << "POSTGRES INSERT: FAIL" << std::endl;
        return 1;
    }

    std::cout << "POSTGRES INSERT: PASS" << std::endl;

    std::cout
        << "INSERTED EVENTS: "
        << store.inserted_events()
        << std::endl;

    if (store.inserted_events() != 1) {
        std::cerr << "POSTGRES INSERT COUNT: FAIL" << std::endl;
        return 1;
    }

    std::cout << "POSTGRES INSERT COUNT: PASS" << std::endl;

    // Verify duplicate protection.
    if (!store.insert_event(event)) {
        std::cerr << "POSTGRES DUPLICATE CHECK: FAIL" << std::endl;
        return 1;
    }

    if (store.inserted_events() != 1) {
        std::cerr << "POSTGRES DUPLICATE PROTECTION: FAIL"
                  << std::endl;
        return 1;
    }

    std::cout << "POSTGRES DUPLICATE PROTECTION: PASS"
              << std::endl;

    store.disconnect();

    if (store.connected()) {
        std::cerr << "POSTGRES DISCONNECT: FAIL" << std::endl;
        return 1;
    }

    std::cout << "POSTGRES DISCONNECT: PASS" << std::endl;

    std::cout << "============================================"
              << std::endl;
    std::cout << "NATIVE POSTGRES INTEGRATION TEST: PASS"
              << std::endl;
    std::cout << "============================================"
              << std::endl;

    return 0;
}

