#pragma once

#include <atomic>
#include <concepts>
#include <cstddef>
#include <memory>
#include <new>
#include <optional>
#include <utility>

namespace nexus::core {

// SPSC (Single-Producer Single-Consumer) Lock-Free Ring Buffer
template <typename T, std::size_t N>
    requires std::destructible<T> && (N > 0)
class ring_buffer {
public:
    using value_type = T;
    using size_type = std::size_t;

    constexpr ring_buffer() noexcept = default;

    ~ring_buffer() noexcept {
        clear();
    }

    // Disable copy/move to simplify lock-free synchronization invariants
    ring_buffer(const ring_buffer&) = delete;
    ring_buffer& operator=(const ring_buffer&) = delete;
    ring_buffer(ring_buffer&&) = delete;
    ring_buffer& operator=(ring_buffer&&) = delete;

    // Enqueue an element (Producer thread only)
    template <typename... Args>
        requires std::constructible_from<T, Args...>
    bool emplace(Args&&... args) {
        const auto current_tail = tail_.load(std::memory_order_relaxed);
        const auto current_head = head_.load(std::memory_order_acquire);

        if (is_full(current_head, current_tail)) {
            return false;
        }

        pointer target = get_pointer(current_tail);
        std::construct_at(target, std::forward<Args>(args)...);

        tail_.store(next_index(current_tail), std::memory_order_release);
        return true;
    }

    bool push(const T& value) {
        return emplace(value);
    }

    bool push(T&& value) {
        return emplace(std::move(value));
    }

    // Dequeue an element (Consumer thread only)
    std::optional<T> pop() {
        const auto current_head = head_.load(std::memory_order_relaxed);
        const auto current_tail = tail_.load(std::memory_order_acquire);

        if (is_empty(current_head, current_tail)) {
            return std::nullopt;
        }

        pointer target = get_pointer(current_head);
        T value = std::move(*target);
        std::destroy_at(target);

        head_.store(next_index(current_head), std::memory_order_release);
        return value;
    }

    [[nodiscard]] size_type capacity() const noexcept { return N; }

    [[nodiscard]] bool empty() const noexcept {
        return head_.load(std::memory_order_relaxed) == tail_.load(std::memory_order_relaxed);
    }

    void clear() noexcept {
        while (pop().has_value()) {}
    }

private:
    // Capacity N requires N+1 slots to distinguish full from empty states
    static constexpr size_type buffer_capacity = N + 1;

    using pointer = T*;

    [[nodiscard]] static constexpr size_type next_index(size_type index) noexcept {
        return (index + 1) % buffer_capacity;
    }

    [[nodiscard]] static constexpr bool is_full(size_type head, size_type tail) noexcept {
        return next_index(tail) == head;
    }

    [[nodiscard]] static constexpr bool is_empty(size_type head, size_type tail) noexcept {
        return head == tail;
    }

    [[nodiscard]] pointer get_pointer(size_type index) noexcept {
        return reinterpret_cast<pointer>(&storage_[index * sizeof(T)]);
    }

    alignas(alignof(T)) std::byte storage_[sizeof(T) * buffer_capacity];

    // Align variables to 64 bytes to prevent False Sharing between producer & consumer CPU caches
    static constexpr std::size_t cacheline_size = 64;

    alignas(cacheline_size) std::atomic<size_type> head_{0};
    alignas(cacheline_size) std::atomic<size_type> tail_{0};
};

} // namespace nexus::core