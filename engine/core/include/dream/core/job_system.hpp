#pragma once

#include <condition_variable>
#include <cstddef>
#include <deque>
#include <functional>
#include <future>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

namespace dream::core {

class JobSystem final {
public:
    explicit JobSystem(std::size_t worker_count = 0);
    JobSystem(const JobSystem&) = delete;
    JobSystem& operator=(const JobSystem&) = delete;
    ~JobSystem();

    template <typename Function, typename... Args>
    auto submit(Function&& function, Args&&... args)
        -> std::future<std::invoke_result_t<Function, Args...>> {
        using Result = std::invoke_result_t<Function, Args...>;
        auto task = std::make_shared<std::packaged_task<Result()>>(
            [fn = std::forward<Function>(function),
             ... captured = std::forward<Args>(args)]() mutable -> Result {
                return std::invoke(fn, std::move(captured)...);
            });
        auto result = task->get_future();
        enqueue([task] { (*task)(); });
        return result;
    }

    // Stops accepting new work, drains queued work, then joins all workers.
    // Lifecycle operations must not be called concurrently or from a worker.
    void shutdown();
    void wait_idle();

    [[nodiscard]] std::size_t worker_count() const noexcept;
    [[nodiscard]] std::size_t pending_count() const;
    [[nodiscard]] bool is_stopping() const;

private:
    void enqueue(std::function<void()> task);
    void worker_loop();

    mutable std::mutex mutex_;
    std::condition_variable work_available_;
    std::condition_variable idle_;
    std::deque<std::function<void()>> queue_;
    std::vector<std::thread> workers_;
    std::size_t active_{0};
    bool stopping_{false};
};

} // namespace dream::core
