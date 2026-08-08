#pragma once

#include <coroutine>
#include <cstdint>
#include <system_error>
#include <sys/epoll.h>
#include <unistd.h>
#include <vector>
#include <stdexcept>

namespace nexus::core {

class io_context {
public:
    io_context() {
        epoll_fd_ = ::epoll_create1(0);
        if (epoll_fd_ == -1) {
            throw std::system_error(errno, std::generic_category(), "Failed to create epoll instance");
        }
    }

    ~io_context() {
        if (epoll_fd_ != -1) {
            ::close(epoll_fd_);
        }
    }

    io_context(const io_context&) = delete;
    io_context& operator=(const io_context&) = delete;
    io_context(io_context&&) = delete;
    io_context& operator=(io_context&&) = delete;

    // Registers a file descriptor with epoll interest events
    void add_event(int fd, uint32_t events, std::coroutine_handle<> handle) {
        ::epoll_event ev{};
        ev.events = events | EPOLLONESHOT;
        ev.data.ptr = handle.address();

        if (::epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &ev) == -1) {
            if (errno == EEXIST) {
                ::epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, fd, &ev);
            } else {
                throw std::system_error(errno, std::generic_category(), "epoll_ctl add/mod failed");
            }
        }
    }

    // Runs a single non-blocking event loop iteration
    std::size_t poll_one(int timeout_ms = 0) {
        constexpr int max_events = 64;
        ::epoll_event events[max_events];

        int nfds = ::epoll_wait(epoll_fd_, events, max_events, timeout_ms);
        if (nfds == -1) {
            if (errno == EINTR) return 0;
            throw std::system_error(errno, std::generic_category(), "epoll_wait failed");
        }

        for (int i = 0; i < nfds; ++i) {
            auto handle_ptr = events[i].data.ptr;
            if (handle_ptr) {
                auto handle = std::coroutine_handle<>::from_address(handle_ptr);
                if (handle && !handle.done()) {
                    handle.resume();
                }
            }
        }

        return static_cast<std::size_t>(nfds);
    }

    // Runs the event loop until stopped
    void run() {
        stopped_ = false;
        while (!stopped_) {
            poll_one(100);
        }
    }

    void stop() noexcept {
        stopped_ = true;
    }

private:
    int epoll_fd_{-1};
    bool stopped_{false};
};

} // namespace nexus::core
