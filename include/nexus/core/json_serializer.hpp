#pragma once

#include <nexus/core/concepts.hpp>
#include <nexus/core/reflect.hpp>
#include <sstream>
#include <string>
#include <tuple>
#include <type_traits>

namespace nexus::core {

class json_serializer {
public:
    template <typename T>
    static std::string serialize(const T& obj) {
        std::ostringstream ss;
        serialize_value(obj, ss);
        return ss.str();
    }

private:
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
        } else if constexpr (std::is_aggregate_v<Decayed>) {
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

} // namespace nexus::core