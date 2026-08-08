#pragma once

#include <tuple>
#include <type_traits>
#include <utility>

namespace nexus::core::reflect {

namespace detail {

// Universal conversion type with non-constexpr member declarations
struct UniversalType {
    template <typename T>
    operator T() const;
};

// Check aggregate parenthesized initialization in C++20 (T(args...))
template <typename T, typename... Args>
concept AggregateConstructible = requires {
    T(std::declval<Args>()...);
};

template <typename T>
constexpr std::size_t count_fields() {
    using Decayed = std::decay_t<T>;
    using U = UniversalType;

    if constexpr (AggregateConstructible<Decayed, U, U, U, U, U, U, U, U>) return 8;
    else if constexpr (AggregateConstructible<Decayed, U, U, U, U, U, U, U>) return 7;
    else if constexpr (AggregateConstructible<Decayed, U, U, U, U, U, U>) return 6;
    else if constexpr (AggregateConstructible<Decayed, U, U, U, U, U>) return 5;
    else if constexpr (AggregateConstructible<Decayed, U, U, U, U>) return 4;
    else if constexpr (AggregateConstructible<Decayed, U, U, U>) return 3;
    else if constexpr (AggregateConstructible<Decayed, U, U>) return 2;
    else if constexpr (AggregateConstructible<Decayed, U>) return 1;
    else return 0;
}

} // namespace detail

template <typename T>
constexpr auto to_tuple(T&& val) {
    using Decayed = std::decay_t<T>;

    if constexpr (requires { typename std::tuple_size<Decayed>::type; }) {
        return val;
    } else if constexpr (std::is_aggregate_v<Decayed>) {
        constexpr std::size_t field_count = detail::count_fields<Decayed>();

        if constexpr (field_count == 4) {
            auto&& [a, b, c, d] = std::forward<T>(val);
            return std::forward_as_tuple(a, b, c, d);
        } else if constexpr (field_count == 3) {
            auto&& [a, b, c] = std::forward<T>(val);
            return std::forward_as_tuple(a, b, c);
        } else if constexpr (field_count == 2) {
            auto&& [a, b] = std::forward<T>(val);
            return std::forward_as_tuple(a, b);
        } else if constexpr (field_count == 1) {
            auto&& [a] = std::forward<T>(val);
            return std::forward_as_tuple(a);
        } else {
            return std::make_tuple();
        }
    } else {
        static_assert(std::is_class_v<Decayed>, "Unsupported type for reflection");
    }
}

} // namespace nexus::core::reflect