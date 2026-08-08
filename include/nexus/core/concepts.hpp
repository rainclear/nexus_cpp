#pragma once

#include <concepts>
#include <ranges>
#include <string>
#include <string_view>
#include <type_traits>

namespace nexus::core::concepts {

// Arithmetic types (integers, floating point, booleans)
template <typename T>
concept Arithmetic = std::is_arithmetic_v<T>;

// String or String View types
template <typename T>
concept StringLike = std::convertible_to<T, std::string_view>;

// Iterable containers excluding strings
template <typename T>
concept Container = std::ranges::input_range<T> && !StringLike<T>;

// Opt-in trait interface for member reflection via structured bindings tuple tuple_size
template <typename T>
concept Reflectable = requires {
    typename std::tuple_size<T>::type;
} || std::is_aggregate_v<T>;

} // namespace nexus::core::concepts