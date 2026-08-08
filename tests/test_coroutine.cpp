#include <catch2/catch_test_macros.hpp>
#include <nexus/core/task.hpp>
#include <string>

nexus::core::task<int> async_add(int a, int b) {
    co_return a + b;
}

nexus::core::task<std::string> async_concat(std::string a, std::string b) {
    int sum = co_await async_add(5, 5);
    co_return a + b + std::to_string(sum);
}

TEST_CASE("C++20 Coroutine task basic operations", "[coroutine]") {
    SECTION("Single coroutine resumption") {
        auto t = async_add(10, 20);
        REQUIRE_FALSE(t.done());
        
        t.resume();
        REQUIRE(t.done());
    }

    SECTION("Nested co_await chaining") {
        auto t = async_concat("Hello ", "World ");
        t.resume();
        
        // Coroutine execution finishes inline during resumption chain
        REQUIRE(t.done());
    }
}