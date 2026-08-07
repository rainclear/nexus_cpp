#include <catch2/catch_test_macros.hpp>
#include <nexus/core/ring_buffer.hpp>
#include <numeric>
#include <string>
#include <thread>
#include <vector>

TEST_CASE("ring_buffer basic single-threaded operations", "[ring_buffer]") {
    nexus::core::ring_buffer<int, 3> rb;

    REQUIRE(rb.empty());
    REQUIRE(rb.capacity() == 3);

    SECTION("push and pop elements") {
        REQUIRE(rb.push(10));
        REQUIRE(rb.push(20));
        REQUIRE(rb.push(30));
        REQUIRE_FALSE(rb.push(40)); // Full

        auto val1 = rb.pop();
        REQUIRE(val1.has_value());
        REQUIRE(*val1 == 10);

        REQUIRE(rb.push(40)); // Re-claim space
        REQUIRE(*rb.pop() == 20);
        REQUIRE(*rb.pop() == 30);
        REQUIRE(*rb.pop() == 40);
        REQUIRE_FALSE(rb.pop().has_value()); // Empty
    }
}

TEST_CASE("ring_buffer SPSC multi-threaded concurrency", "[ring_buffer]") {
    constexpr std::size_t capacity = 1024;
    constexpr std::size_t item_count = 100000;

    nexus::core::ring_buffer<std::size_t, capacity> rb;
    std::vector<std::size_t> consumed_items;
    consumed_items.reserve(item_count);

    // Producer Thread
    std::thread producer([&]() {
        for (std::size_t i = 0; i < item_count; ++i) {
            while (!rb.push(i)) {
                std::this_thread::yield();
            }
        }
    });

    // Consumer Thread
    std::thread consumer([&]() {
        for (std::size_t i = 0; i < item_count; ++i) {
            std::optional<std::size_t> val;
            while (!(val = rb.pop())) {
                std::this_thread::yield();
            }
            consumed_items.push_back(*val);
        }
    });

    producer.join();
    consumer.join();

    REQUIRE(consumed_items.size() == item_count);
    for (std::size_t i = 0; i < item_count; ++i) {
        REQUIRE(consumed_items[i] == i);
    }
}