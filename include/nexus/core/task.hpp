#pragma once

#include <coroutine>
#include <exception>
#include <stdexcept>
#include <utility>
#include <variant>

namespace nexus::core {

// 1. Forward declaration of primary template
template <typename T = void>
class [[nodiscard]] task;

// 2. Explicit specialization for task<void>
template <>
class [[nodiscard]] task<void> {
public:
    struct promise_type {
        std::exception_ptr exception{nullptr};
        std::coroutine_handle<> continuation{nullptr};

        task<void> get_return_object() noexcept {
            return task<void>{std::coroutine_handle<promise_type>::from_promise(*this)};
        }

        std::suspend_always initial_suspend() noexcept { return {}; }

        struct final_awaiter {
            bool await_ready() noexcept { return false; }
            
            std::coroutine_handle<> await_suspend(std::coroutine_handle<promise_type> h) noexcept {
                if (h.promise().continuation) {
                    return h.promise().continuation;
                }
                return std::noop_coroutine();
            }

            void await_resume() noexcept {}
        };

        final_awaiter final_suspend() noexcept { return {}; }

        void return_void() noexcept {}

        void unhandled_exception() noexcept {
            exception = std::current_exception();
        }
    };

    using handle_type = std::coroutine_handle<promise_type>;

    explicit task(handle_type h) noexcept : handle_(h) {}

    ~task() {
        if (handle_) handle_.destroy();
    }

    task(const task&) = delete;
    task& operator=(const task&) = delete;

    task(task&& other) noexcept : handle_(std::exchange(other.handle_, nullptr)) {}

    task& operator=(task&& other) noexcept {
        if (this != &other) {
            if (handle_) handle_.destroy();
            handle_ = std::exchange(other.handle_, nullptr);
        }
        return *this;
    }

    bool await_ready() const noexcept {
        return !handle_ || handle_.done();
    }

    std::coroutine_handle<> await_suspend(std::coroutine_handle<> awaiting_coroutine) noexcept {
        handle_.promise().continuation = awaiting_coroutine;
        return handle_;
    }

    void await_resume() {
        if (!handle_) {
            throw std::runtime_error("Attempting to await an empty task");
        }
        if (handle_.promise().exception) {
            std::rethrow_exception(handle_.promise().exception);
        }
    }

    void resume() {
        if (handle_ && !handle_.done()) {
            handle_.resume();
        }
    }

    [[nodiscard]] bool done() const noexcept {
        return !handle_ || handle_.done();
    }

private:
    handle_type handle_{nullptr};
};

// 3. Primary template for non-void types
template <typename T>
class [[nodiscard]] task {
public:
    struct promise_type;
    using handle_type = std::coroutine_handle<promise_type>;

    struct promise_type {
        std::variant<std::monostate, T, std::exception_ptr> result;
        std::coroutine_handle<> continuation{nullptr};

        task get_return_object() noexcept {
            return task{handle_type::from_promise(*this)};
        }

        std::suspend_always initial_suspend() noexcept { return {}; }

        struct final_awaiter {
            bool await_ready() noexcept { return false; }
            
            std::coroutine_handle<> await_suspend(handle_type h) noexcept {
                if (h.promise().continuation) {
                    return h.promise().continuation;
                }
                return std::noop_coroutine();
            }

            void await_resume() noexcept {}
        };

        final_awaiter final_suspend() noexcept { return {}; }

        template <typename Value>
            requires std::convertible_to<Value, T>
        void return_value(Value&& val) {
            result.template emplace<1>(std::forward<Value>(val));
        }

        void unhandled_exception() noexcept {
            result.template emplace<2>(std::current_exception());
        }
    };

    explicit task(handle_type h) noexcept : handle_(h) {}

    ~task() {
        if (handle_) {
            handle_.destroy();
        }
    }

    task(const task&) = delete;
    task& operator=(const task&) = delete;

    task(task&& other) noexcept : handle_(std::exchange(other.handle_, nullptr)) {}
    
    task& operator=(task&& other) noexcept {
        if (this != &other) {
            if (handle_) handle_.destroy();
            handle_ = std::exchange(other.handle_, nullptr);
        }
        return *this;
    }

    bool await_ready() const noexcept {
        return !handle_ || handle_.done();
    }

    std::coroutine_handle<> await_suspend(std::coroutine_handle<> awaiting_coroutine) noexcept {
        handle_.promise().continuation = awaiting_coroutine;
        return handle_;
    }

    T await_resume() {
        if (!handle_) {
            throw std::runtime_error("Attempting to await an empty task");
        }
        
        auto& res = handle_.promise().result;
        if (res.index() == 2) {
            std::rethrow_exception(std::get<2>(res));
        }
        return std::get<1>(std::move(res));
    }

    void resume() {
        if (handle_ && !handle_.done()) {
            handle_.resume();
        }
    }

    [[nodiscard]] bool done() const noexcept {
        return !handle_ || handle_.done();
    }

private:
    handle_type handle_{nullptr};
};

} // namespace nexus::core