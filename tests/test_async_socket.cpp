#include <catch2/catch_test_macros.hpp>
#include <nexus/core/async_socket.hpp>
#include <nexus/core/io_context.hpp>
#include <nexus/core/task.hpp>
#include <array>
#include <cstring>
#include <sys/socket.h>

nexus::core::task<void> echo_server_coroutine(nexus::core::async_socket socket) {
    std::array<std::byte, 128> buffer{};
    
    // Non-blocking wait & read
    auto [bytes_read, read_err] = co_await socket.async_read(buffer);
    REQUIRE_FALSE(read_err);
    REQUIRE(bytes_read > 0);

    // Echo back async write
    auto [bytes_written, write_err] = co_await socket.async_write(std::span{buffer.data(), bytes_read});
    REQUIRE_FALSE(write_err);
    REQUIRE(bytes_written == bytes_read);

    co_return;
}

TEST_CASE("Async Socket Coroutine Echo Loop", "[async_socket]") {
    int sv[2];
    REQUIRE(::socketpair(AF_UNIX, SOCK_STREAM, 0, sv) == 0);

    nexus::core::io_context ctx;
    nexus::core::async_socket server_sock(sv[0], ctx);

    // Launch server coroutine
    auto server_task = echo_server_coroutine(std::move(server_sock));
    server_task.resume(); // Reaches co_await socket.async_read and suspends

    // Client writes to socket
    const char* message = "Hello Coroutine!";
    ::write(sv[1], message, std::strlen(message));

    // Poll event loop until the server coroutine completes both read and write
    while (!server_task.done()) {
        ctx.poll_one(10);
    }

    // Client reads echoed response
    char response[128]{};
    ssize_t n = ::read(sv[1], response, sizeof(response));
    REQUIRE(n == static_cast<ssize_t>(std::strlen(message)));
    REQUIRE(std::string_view(response, n) == "Hello Coroutine!");

    ::close(sv[1]);
}