#include <catch2/catch_test_macros.hpp>
#include <nexus/core/thread_pool.hpp>
#include <atomic>
#include <numeric>
#include <vector>

TEST_CASE("thread_pool basic operations", "[thread_pool]") {
    nexus::core::thread_pool pool(4);

    REQUIRE(pool.worker_count() == 4);

    SECTION("Enqueue async tasks and retrieve futures") {
        auto fut1 = pool.enqueue([]() { return 42; });
        auto fut2 = pool.enqueue([](int a, int b) { return a + b; }, 10, 20);

        REQUIRE(fut1.get() == 42);
        REQUIRE(fut2.get() == 30);
    }

    SECTION("Concurrent execution of multiple tasks") {
        constexpr int task_count = 100;
        std::atomic<int> counter{0};
        std::vector<std::future<void>> futures;
        futures.reserve(task_count);

        for (int i = 0; i < task_count; ++i) {
            futures.push_back(pool.enqueue([&counter]() {
                counter.fetch_add(1, std::memory_order_relaxed);
            }));
        }

        for (auto& fut : futures) {
            fut.get();
        }

        REQUIRE(counter.load() == task_count);
    }
}