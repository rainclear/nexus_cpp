#include <catch2/catch_test_macros.hpp>
#include <nexus/core/json_serializer.hpp>
#include <nexus/core/fixed_vector.hpp>
#include <string>
#include <vector>

struct UserProfile {
    int id;
    std::string name;
};

TEST_CASE("JSON Serializer - Primitive and Container Types", "[serializer]") {
    SECTION("Arithmetic types") {
        REQUIRE(nexus::core::json_serializer::serialize(42) == "42");
        REQUIRE(nexus::core::json_serializer::serialize(true) == "true");
    }

    SECTION("Strings and Containers") {
        REQUIRE(nexus::core::json_serializer::serialize(std::string("nexus")) == "\"nexus\"");
        
        std::vector<int> nums = {1, 2, 3};
        REQUIRE(nexus::core::json_serializer::serialize(nums) == "[1,2,3]");
    }
}

TEST_CASE("JSON Serializer - Reflection on Aggregate Types", "[serializer]") {
    UserProfile user{101, "Alice"};
    std::string json = nexus::core::json_serializer::serialize(user);
    
    REQUIRE(json == "{\"field_0\":101,\"field_1\":\"Alice\"}");
}