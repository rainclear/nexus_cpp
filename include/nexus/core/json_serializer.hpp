#pragma once

#include <nexus/core/concepts.hpp>
#include <nexus/core/reflect.hpp>
#include <concepts>
#include <sstream>
#include <string>
#include <tuple>
#include <type_traits>

namespace nexus::core {

class json_serializer;

// Helper context builder passed into custom serialize_members methods
class object_builder {
public:
    explicit object_builder(std::ostringstream& ss) : ss_(ss) {}

    template <typename Value>
    void field(std::string_view key, const Value& val);

private:
    std::ostringstream& ss_;
    bool first_{true};
};

namespace detail {

// Concepts declared at namespace scope
template <typename T>
concept CustomSerializable = requires(const T& obj, object_builder& builder) {
    obj.serialize_members(builder);
};

} // namespace detail

class json_serializer {
public:
    template <typename T>
    static std::string serialize(const T& obj) {
        std::ostringstream ss;
        serialize_value(obj, ss);
        return ss.str();
    }

private:
    // Correct non-template friend declaration
    friend class object_builder;

    template <typename T>
    static void serialize_value(const T& val, std::ostringstream& ss) {
        using Decayed = std::decay_t<T>;

        if constexpr (std::is_same_v<Decayed, bool>) {
            ss << (val ? "true" : "false");
        } else if constexpr (concepts::Arithmetic<Decayed>) {
            ss << val;
        } else if constexpr (concepts::StringLike<Decayed>) {
            ss << '"' << std::string_view(val) << '"';
        } else if constexpr (concepts::Container<Decayed>) {
            ss << '[';
            bool first = true;
            for (const auto& item : val) {
                if (!first) ss << ',';
                serialize_value(item, ss);
                first = false;
            }
            ss << ']';
        } else if constexpr (detail::CustomSerializable<Decayed>) {
            // Custom named fields
            ss << '{';
            object_builder builder(ss);
            val.serialize_members(builder);
            ss << '}';
        } else if constexpr (std::is_aggregate_v<Decayed>) {
            // Positional fallback aggregate reflection
            ss << '{';
            auto tuple = reflect::to_tuple(val);
            
            std::size_t idx = 0;
            std::apply([&ss, &idx](auto&&... args) {
                auto write_field = [&](auto&& arg) {
                    if (idx > 0) ss << ',';
                    ss << "\"field_" << idx << "\":";
                    serialize_value(arg, ss);
                    ++idx;
                };
                (write_field(args), ...);
            }, tuple);

            ss << '}';
        }
    }
};

template <typename Value>
inline void object_builder::field(std::string_view key, const Value& val) {
    if (!first_) ss_ << ',';
    ss_ << '"' << key << "\":";
    json_serializer::serialize_value(val, ss_);
    first_ = false;
}

} // namespace nexus::core