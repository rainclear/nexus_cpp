#pragma once

#include <cstddef>
#include <concepts>
#include <initializer_list>
#include <memory>
#include <new>
#include <stdexcept>
#include <utility>

namespace nexus::core {

template <typename T, std::size_t N>
    requires std::destructible<T> && (N > 0)
class fixed_vector {
public:
    using value_type = T;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using reference = T&;
    using const_reference = const T&;
    using pointer = T*;
    using const_pointer = const T*;

    // Iterators (Contiguous Iterator requirement)
    using iterator = pointer;
    using const_iterator = const_pointer;

    constexpr fixed_vector() noexcept = default;

    constexpr ~fixed_vector() noexcept {
        clear();
    }

    // Copy Constructor
    constexpr fixed_vector(const fixed_vector& other) {
        for (size_type i = 0; i < other.size_; ++i) {
            push_back(other[i]);
        }
    }

    // Move Constructor
    constexpr fixed_vector(fixed_vector&& other) noexcept(std::is_nothrow_move_constructible_v<T>) {
        for (size_type i = 0; i < other.size_; ++i) {
            emplace_back(std::move(other[i]));
        }
        other.clear();
    }

    // Copy Assignment Operator
    constexpr fixed_vector& operator=(const fixed_vector& other) {
        if (this != &other) {
            clear();
            for (size_type i = 0; i < other.size_; ++i) {
                push_back(other[i]);
            }
        }
        return *this;
    }

    // Move Assignment Operator
    constexpr fixed_vector& operator=(fixed_vector&& other) noexcept(std::is_nothrow_move_constructible_v<T>) {
        if (this != &other) {
            clear();
            for (size_type i = 0; i < other.size_; ++i) {
                emplace_back(std::move(other[i]));
            }
            other.clear();
        }
        return *this;
    }

    // Initializer List Constructor
    constexpr fixed_vector(std::initializer_list<T> init) {
        for (const auto& item : init) {
            push_back(item);
        }
    }

    // Iterators Support
    [[nodiscard]] constexpr iterator begin() noexcept { return data(); }
    [[nodiscard]] constexpr const_iterator begin() const noexcept { return data(); }
    [[nodiscard]] constexpr const_iterator cbegin() const noexcept { return data(); }

    [[nodiscard]] constexpr iterator end() noexcept { return data() + size_; }
    [[nodiscard]] constexpr const_iterator end() const noexcept { return data() + size_; }
    [[nodiscard]] constexpr const_iterator cend() const noexcept { return data() + size_; }

    // Element Access
    [[nodiscard]] constexpr pointer data() noexcept {
        return reinterpret_cast<pointer>(&storage_[0]);
    }

    [[nodiscard]] constexpr const_pointer data() const noexcept {
        return reinterpret_cast<const_pointer>(&storage_[0]);
    }

    [[nodiscard]] constexpr reference operator[](size_type index) noexcept {
        return data()[index];
    }

    [[nodiscard]] constexpr const_reference operator[](size_type index) const noexcept {
        return data()[index];
    }

    [[nodiscard]] constexpr reference at(size_type index) {
        if (index >= size_) {
            throw std::out_of_range("fixed_vector::at out of range");
        }
        return data()[index];
    }

    [[nodiscard]] constexpr const_reference at(size_type index) const {
        if (index >= size_) {
            throw std::out_of_range("fixed_vector::at out of range");
        }
        return data()[index];
    }

    [[nodiscard]] constexpr reference front() noexcept { return data()[0]; }
    [[nodiscard]] constexpr const_reference front() const noexcept { return data()[0]; }

    [[nodiscard]] constexpr reference back() noexcept { return data()[size_ - 1]; }
    [[nodiscard]] constexpr const_reference back() const noexcept { return data()[size_ - 1]; }

    // Capacity Methods
    [[nodiscard]] constexpr size_type size() const noexcept { return size_; }
    [[nodiscard]] constexpr size_type capacity() const noexcept { return N; }
    [[nodiscard]] constexpr bool empty() const noexcept { return size_ == 0; }
    [[nodiscard]] constexpr bool full() const noexcept { return size_ == N; }

    // Modifiers
    template <typename... Args>
        requires std::constructible_from<T, Args...>
    constexpr reference emplace_back(Args&&... args) {
        if (size_ >= N) {
            throw std::out_of_range("fixed_vector capacity exceeded");
        }
        pointer target = data() + size_;
        std::construct_at(target, std::forward<Args>(args)...);
        ++size_;
        return *target;
    }

    constexpr void push_back(const T& value) {
        emplace_back(value);
    }

    constexpr void push_back(T&& value) {
        emplace_back(std::move(value));
    }

    constexpr void pop_back() {
        if (size_ == 0) return;
        --size_;
        std::destroy_at(data() + size_);
    }

    constexpr iterator erase(const_iterator pos) {
        auto idx = static_cast<size_type>(pos - cbegin());
        if (idx >= size_) {
            throw std::out_of_range("fixed_vector::erase out of range");
        }
        pointer p = data() + idx;
        for (pointer it = p; it != data() + size_ - 1; ++it) {
            *it = std::move(*(it + 1));
        }
        pop_back();
        return data() + idx;
    }

    constexpr void clear() noexcept {
        while (size_ > 0) {
            pop_back();
        }
    }

private:
    alignas(alignof(T)) std::byte storage_[sizeof(T) * N];
    size_type size_{0};
};

} // namespace nexus::core