#pragma once

#include <nexus/core/io_context.hpp>
#include <coroutine>
#include <cstddef>
#include <fcntl.h>
#include <span>
#include <system_error>
#include <sys/socket.h>
#include <unistd.h>
#include <utility>

namespace nexus::core {

class async_socket {
public:
    explicit async_socket(int fd, io_context& ctx) : fd_(fd), ctx_(&ctx) {
        set_nonblocking(fd_);
    }

    ~async_socket() {
        if (fd_ != -1) {
            ::close(fd_);
        }
    }

    async_socket(const async_socket&) = delete;
    async_socket& operator=(const async_socket&) = delete;

    async_socket(async_socket&& other) noexcept 
        : fd_(std::exchange(other.fd_, -1)), ctx_(other.ctx_) {}

    async_socket& operator=(async_socket&& other) noexcept {
        if (this != &other) {
            if (fd_ != -1) ::close(fd_);
            fd_ = std::exchange(other.fd_, -1);
            ctx_ = other.ctx_;
        }
        return *this;
    }

    [[nodiscard]] int native_handle() const noexcept { return fd_; }

    // Coroutine Awaitable for non-blocking Async Read
    struct read_awaiter {
        int fd;
        io_context& ctx;
        std::span<std::byte> buffer;

        bool await_ready() const noexcept { return false; }

        void await_suspend(std::coroutine_handle<> h) {
            ctx.add_event(fd, EPOLLIN, h);
        }

        std::pair<std::size_t, std::error_code> await_resume() noexcept {
            ssize_t bytes_read = ::read(fd, buffer.data(), buffer.size());
            if (bytes_read >= 0) {
                return {static_cast<std::size_t>(bytes_read), {}};
            }
            return {0, std::make_error_code(static_cast<std::errc>(errno))};
        }
    };

    // Coroutine Awaitable for non-blocking Async Write
    struct write_awaiter {
        int fd;
        io_context& ctx;
        std::span<const std::byte> buffer;

        bool await_ready() const noexcept { return false; }

        void await_suspend(std::coroutine_handle<> h) {
            ctx.add_event(fd, EPOLLOUT, h);
        }

        std::pair<std::size_t, std::error_code> await_resume() noexcept {
            ssize_t bytes_written = ::write(fd, buffer.data(), buffer.size());
            if (bytes_written >= 0) {
                return {static_cast<std::size_t>(bytes_written), {}};
            }
            return {0, std::make_error_code(static_cast<std::errc>(errno))};
        }
    };

    auto async_read(std::span<std::byte> buffer) {
        return read_awaiter{fd_, *ctx_, buffer};
    }

    auto async_write(std::span<const std::byte> buffer) {
        return write_awaiter{fd_, *ctx_, buffer};
    }

private:
    static void set_nonblocking(int fd) {
        int flags = ::fcntl(fd, F_GETFL, 0);
        ::fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    }

    int fd_{-1};
    io_context* ctx_{nullptr};
};

} // namespace nexus::core