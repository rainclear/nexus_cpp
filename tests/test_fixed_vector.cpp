#include <catch2/catch_test_macros.hpp>
#include <nexus/core/fixed_vector.hpp>
#include <algorithm>
#include <numeric>
#include <ranges>
#include <string>

TEST_CASE("fixed_vector basic operations", "[fixed_vector]") {
    nexus::core::fixed_vector<int, 5> vec;

    REQUIRE(vec.empty());
    REQUIRE(vec.capacity() == 5);
    REQUIRE(vec.size() == 0);

    SECTION("push_back and element access") {
        vec.push_back(10);
        vec.push_back(20);

        REQUIRE(vec.size() == 2);
        REQUIRE(vec[0] == 10);
        REQUIRE(vec[1] == 20);
        REQUIRE(vec.at(0) == 10);
        REQUIRE_THROWS_AS(vec.at(2), std::out_of_range);
    }

    SECTION("capacity limits") {
        for (int i = 0; i < 5; ++i) {
            vec.push_back(i);
        }
        REQUIRE(vec.full());
        REQUIRE_THROWS_AS(vec.push_back(99), std::out_of_range);
    }
}

TEST_CASE("fixed_vector iterators and C++20 ranges", "[fixed_vector]") {
    nexus::core::fixed_vector<int, 5> vec = {5, 2, 8, 1, 3};

    SECTION("Range-based for loop and STL algorithm compatibility") {
        std::sort(vec.begin(), vec.end());
        REQUIRE(vec[0] == 1);
        REQUIRE(vec[4] == 8);

        int sum = std::accumulate(vec.cbegin(), vec.cend(), 0);
        REQUIRE(sum == 19);
    }

    SECTION("C++20 Ranges pipeline") {
        auto even_squares = vec 
            | std::views::filter([](int n) { return n % 2 == 0; })
            | std::views::transform([](int n) { return n * n; });

        nexus::core::fixed_vector<int, 5> results;
        for (int val : even_squares) {
            results.push_back(val);
        }

        REQUIRE(results.size() == 2);
        REQUIRE(results[0] == 4);   // 2^2
        REQUIRE(results[1] == 64);  // 8^2
    }

    SECTION("Erase operation") {
        vec.erase(vec.begin() + 1); // remove element '2'
        REQUIRE(vec.size() == 4);
        REQUIRE(vec[1] == 8);
    }
}