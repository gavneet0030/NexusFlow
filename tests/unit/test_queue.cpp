#include "core/queue/bounded_mpmc_queue.hpp"

#include <atomic>
#include <chrono>
#include <future>
#include <iostream>
#include <thread>
#include <vector>
#include <utility>

using nexusflow::BoundedMPMCQueue;
using namespace std::chrono_literals;

namespace {

bool test_zero_capacity_is_rejected()
{
    try {
        BoundedMPMCQueue<int> q(0);
    } catch (const std::invalid_argument&) {
        return true;
    } catch (...) {
        return false;
    }
    return false;
}

bool test_fifo_and_capacity()
{
    BoundedMPMCQueue<int> q(2);
    if (q.capacity() != 2 || !q.try_push(10) ||
        !q.try_push(20) || q.try_push(30) || q.size() != 2) {
        return false;
    }

    int value = 0;
    return q.try_pop(value) && value == 10 &&
           q.try_pop(value) && value == 20 &&
           !q.try_pop(value) && q.empty();
}

bool test_close_and_drain()
{
    BoundedMPMCQueue<int> q(2);
    if (!q.push(1) || !q.push(2)) {
        return false;
    }

    q.close();
    int value = 0;

    return q.closed() &&
           !q.push(3) &&
           !q.try_push(3) &&
           q.pop(value) && value == 1 &&
           q.pop(value) && value == 2 &&
           !q.pop(value) &&
           q.empty();
}

bool test_timed_pop()
{
    BoundedMPMCQueue<int> q(2);
    int value = 0;

    const auto start = std::chrono::steady_clock::now();
    if (q.pop_for(value, 30ms)) {
        return false;
    }
    if (std::chrono::steady_clock::now() - start < 20ms) {
        return false;
    }

    if (!q.try_push(42) || !q.pop_for(value, 100ms) || value != 42) {
        return false;
    }

    q.close();
    return !q.pop_for(value, 100ms);
}

bool test_blocked_consumer_wakes_on_close()
{
    BoundedMPMCQueue<int> q(1);
    std::promise<bool> promise;
    auto result = promise.get_future();

    std::thread consumer([&]() {
        int value = 0;
        promise.set_value(q.pop(value));
    });

    q.close();

    const auto status = result.wait_for(2s);
    if (status != std::future_status::ready) {
        // Keep queue alive until the consumer exits; never detach it.
        consumer.join();
        return false;
    }

    consumer.join();
    return !result.get();
}

bool test_blocked_producer_wakes_on_close()
{
    BoundedMPMCQueue<int> q(1);
    if (!q.push(1)) {
        return false;
    }

    std::promise<void> startedPromise;
    auto started = startedPromise.get_future();
    std::promise<bool> resultPromise;
    auto result = resultPromise.get_future();

    std::thread producer([&]() {
        startedPromise.set_value();
        resultPromise.set_value(q.push(2));
    });

    if (started.wait_for(2s) != std::future_status::ready) {
        q.close();
        producer.join();
        return false;
    }

    q.close();
    const auto status = result.wait_for(2s);
    if (status != std::future_status::ready) {
        producer.join();
        return false;
    }

    producer.join();

    int value = 0;
    return !result.get() && q.pop(value) && value == 1;
}

bool test_concurrent_producers_and_consumers()
{
    constexpr int producerCount = 4;
    constexpr int itemsPerProducer = 500;
    constexpr int total = producerCount * itemsPerProducer;

    BoundedMPMCQueue<int> q(64);
    std::vector<std::atomic<int>> counts(total);
    for (auto& count : counts) {
        count.store(0);
    }

    std::atomic<bool> failed{false};
    std::vector<std::thread> consumers;

    for (int c = 0; c < 4; ++c) {
        consumers.emplace_back([&]() {
            int value = 0;
            while (q.pop(value)) {
                if (value < 0 || value >= total) {
                    failed.store(true);
                    continue;
                }
                if (counts[value].fetch_add(1) != 0) {
                    failed.store(true);
                }
            }
        });
    }

    std::vector<std::thread> producers;
    for (int p = 0; p < producerCount; ++p) {
        producers.emplace_back([&, p]() {
            for (int i = 0; i < itemsPerProducer; ++i) {
                if (!q.push(p * itemsPerProducer + i)) {
                    failed.store(true);
                    return;
                }
            }
        });
    }

    for (auto& producer : producers) {
        producer.join();
    }
    q.close();

    for (auto& consumer : consumers) {
        consumer.join();
    }

    if (failed.load()) {
        return false;
    }
    for (const auto& count : counts) {
        if (count.load() != 1) {
            return false;
        }
    }
    return true;
}

} // namespace

int main()
{
    const std::vector<std::pair<const char*, bool (*)()>> tests = {
        {"zero capacity is rejected", test_zero_capacity_is_rejected},
        {"FIFO and capacity", test_fifo_and_capacity},
        {"close and drain", test_close_and_drain},
        {"timed pop", test_timed_pop},
        {"blocked consumer wakes on close", test_blocked_consumer_wakes_on_close},
        {"blocked producer wakes on close", test_blocked_producer_wakes_on_close},
        {"concurrent producers and consumers", test_concurrent_producers_and_consumers},
    };

    bool allPassed = true;
    for (const auto& test : tests) {
        bool passed = false;
        try {
            passed = test.second();
        } catch (...) {
            passed = false;
        }
        std::cout << (passed ? "[PASS] " : "[FAIL] ")
                  << test.first << '\n';
        allPassed = allPassed && passed;
    }

    std::cout << (allPassed ? "RESULT: PASS\n" : "RESULT: FAIL\n");
    return allPassed ? 0 : 1;
}
