#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <thread>

namespace {
std::atomic<bool> running{true};

void signal_handler(int) {
    running.store(false, std::memory_order_relaxed);
}

int read_positive_env(const char* name, int fallback) {
    const char* value = std::getenv(name);

    if (value == nullptr) {
        return fallback;
    }

    try {
        const int parsed = std::stoi(value);
        return parsed > 0 ? parsed : fallback;
    } catch (...) {
        return fallback;
    }
}
}

int main() {
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    const int workers = read_positive_env("NEXUSFLOW_WORKERS", 4);

    std::cout << "============================================================\n";
    std::cout << "              NEXUSFLOW KUBERNETES SERVICE\n";
    std::cout << "============================================================\n";
    std::cout << "Environment     : "
              << (std::getenv("NEXUSFLOW_ENV")
                      ? std::getenv("NEXUSFLOW_ENV")
                      : "unknown")
              << '\n';
    std::cout << "Scheduler       : "
              << (std::getenv("NEXUSFLOW_SCHEDULER_MODE")
                      ? std::getenv("NEXUSFLOW_SCHEDULER_MODE")
                      : "adaptive")
              << '\n';
    std::cout << "Workers         : " << workers << '\n';
    std::cout << "Metrics         : "
              << (std::getenv("NEXUSFLOW_METRICS_ENABLED")
                      ? std::getenv("NEXUSFLOW_METRICS_ENABLED")
                      : "false")
              << '\n';
    std::cout << "Tracing         : "
              << (std::getenv("NEXUSFLOW_TRACING_ENABLED")
                      ? std::getenv("NEXUSFLOW_TRACING_ENABLED")
                      : "false")
              << '\n';
    std::cout << "Status          : RUNNING\n";
    std::cout << "============================================================\n";
    std::cout.flush();

    while (running.load(std::memory_order_relaxed)) {
        std::this_thread::sleep_for(std::chrono::seconds(5));

        std::cout << "NexusFlow service heartbeat: RUNNING\n";
        std::cout.flush();
    }

    std::cout << "NexusFlow service shutdown requested\n";
    std::cout << "NexusFlow service stopped\n";
    std::cout.flush();

    return 0;
}
