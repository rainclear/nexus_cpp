#include <nexus/core/async_socket.hpp>
#include <nexus/core/fixed_vector.hpp>
#include <nexus/core/io_context.hpp>
#include <nexus/core/json_serializer.hpp>
#include <nexus/core/task.hpp>
#include <nexus/core/thread_pool.hpp>

#include <arpa/inet.h>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>

struct EchoResponse {
    int status_code;
    std::string message;

    // Zero macro opt-in for custom field names
    template <typename Builder>
    void serialize_members(Builder& b) const {
        b.field("status_code", status_code);
        b.field("message", message);
    }
};

// Returns detach_task so the coroutine frame is self-managed
nexus::core::detach_task handle_client(nexus::core::async_socket socket) {
    nexus::core::fixed_vector<std::byte, 1024> recv_buf(1024);
    
    // 1. Asynchronously receive raw request bytes
    auto [bytes_read, read_err] = co_await socket.async_read(recv_buf);
    if (read_err || bytes_read == 0) {
        co_return;
    }

    // 2. Process payload and build DTO
    EchoResponse resp{200, "Request processed via Nexus C++20 Engine"};

    // 3. Phase 2 Compile-Time Serialization
    std::string body = nexus::core::json_serializer::serialize(resp);

    // 4. Construct HTTP Response Header
    std::string http_resp = 
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: " + std::to_string(body.size()) + "\r\n"
        "Connection: close\r\n\r\n" + body;

    // 5. Asynchronously transmit response
    co_await socket.async_write(
        std::span{reinterpret_cast<const std::byte*>(http_resp.data()), http_resp.size()}
    );

    co_return;
}

int main() {
    nexus::core::io_context io_ctx;
    nexus::core::thread_pool pool(4);

    int listen_fd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    int opt = 1;
    ::setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (::bind(listen_fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        std::cerr << "Failed to bind to port 8080\n";
        return 1;
    }

    ::listen(listen_fd, SOMAXCONN);
    std::cout << "Nexus C++ Server listening on http://127.0.0.1:8080...\n";

    // Non-blocking connection accept loop
    while (true) {
        sockaddr_in client_addr{};
        socklen_t client_len = sizeof(client_addr);
        int client_fd = ::accept4(listen_fd, reinterpret_cast<sockaddr*>(&client_addr), &client_len, SOCK_NONBLOCK);

        if (client_fd >= 0) {
            // Spawn detached coroutine handler safely
            pool.enqueue([client_fd, &io_ctx]() {
                nexus::core::async_socket sock(client_fd, io_ctx);
                handle_client(std::move(sock)); // Auto-starts and manages frame
            });
        }

        io_ctx.poll_one(10);
    }

    return 0;
}