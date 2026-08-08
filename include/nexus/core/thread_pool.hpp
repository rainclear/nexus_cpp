#pragma once

#include <condition_variable>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <stop_token>
#include <thread>
#include <type_traits>
#include <vector>

namespace nexus::core {

class thread_pool {
public:
    // Constructor: specifies thread count, defaults to hardware concurrency limit
    explicit thread_pool(std::size_t threads_count = std::thread::hardware_concurrency()) {
        const std::size_t worker_count = (threads_count == 0) ? 4 : threads_count;
        workers_.reserve(worker_count);
        for (std::size_t i = 0; i < worker_count; ++i) {
            // std::jthread automatically passes std::stop_token into the thread function
            workers_.emplace_back([this](std::stop_token stop_tok) {
                worker_loop(stop_tok);
            });
        }
    }

    // Destructor: relies on jthread RAII and request_stop for graceful shutdown
    ~thread_pool() {
        shutdown();
    }

    // Non-copyable and non-movable
    thread_pool(const thread_pool&) = delete;
    thread_pool& operator=(const thread_pool&) = delete;
    thread_pool(thread_pool&&) = delete;
    thread_pool& operator=(thread_pool&&) = delete;

    // Submits a task to the pool and returns a std::future for retrieving the result
    template <typename F, typename... Args>
    auto enqueue(F&& f, Args&&... args) 
        -> std::future<std::invoke_result_t<F, Args...>> {
        
        using return_type = std::invoke_result_t<F, Args...>;

        // Package callable and arguments into a shared packaged_task
        auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );

        std::future<return_type> res = task->get_future();
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            if (stopping_) {
                throw std::runtime_error("enqueue on stopped thread_pool");
            }

            // Store type-erased task wrapper
            tasks_.emplace([task]() { (*task)(); });
        }

        // Notify one waiting worker thread
        cv_.notify_one();
        return res;
    }

    // Manually performs a graceful shutdown of the pool
    void shutdown() noexcept {
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            if (stopping_) return;
            stopping_ = true;
        }

        // Signal stop request to all workers via jthread stop_tokens
        for (auto& worker : workers_) {
            worker.request_stop();
        }

        // Wake up all threads waiting on condition_variable to exit loops
        cv_.notify_all();

        // workers_ vector cleanup automatically joins threads during destruction
    }

    [[nodiscard]] std::size_t worker_count() const noexcept {
        return workers_.size();
    }

private:
    void worker_loop(std::stop_token stop_tok) {
        while (!stop_tok.stop_requested()) {
            std::function<void()> task;

            {
                std::unique_lock<std::mutex> lock(queue_mutex_);
                // Wait until work is available, pool is shutting down, or stop is requested
                cv_.wait(lock, [this, &stop_tok] {
                    return stopping_ || !tasks_.empty() || stop_tok.stop_requested();
                });

                if ((stopping_ || stop_tok.stop_requested()) && tasks_.empty()) {
                    return;
                }

                task = std::move(tasks_.front());
                tasks_.pop();
            }

            // Execute the task outside the lock
            task();
        }
    }

    std::vector<std::jthread> workers_;
    std::queue<std::function<void()>> tasks_;

    mutable std::mutex queue_mutex_;
    std::condition_variable cv_;
    bool stopping_{false};
};

} // namespace nexus::core