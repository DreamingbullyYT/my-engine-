#include <dream/core/job_system.hpp>

#include <algorithm>

namespace dream::core {

JobSystem::JobSystem(std::size_t worker_count) {
    if (worker_count == 0) {
        worker_count = std::thread::hardware_concurrency();
        if (worker_count == 0) worker_count = 1;
    }

    workers_.reserve(worker_count);
    try {
        for (std::size_t i = 0; i < worker_count; ++i)
            workers_.emplace_back([this] { worker_loop(); });
    } catch (...) {
        shutdown();
        throw;
    }
}

JobSystem::~JobSystem() {
    shutdown();
}

void JobSystem::enqueue(std::function<void()> task) {
    {
        std::lock_guard lock(mutex_);
        if (stopping_) throw std::runtime_error("JobSystem is stopping");
        queue_.push_back(std::move(task));
    }
    work_available_.notify_one();
}

void JobSystem::worker_loop() {
    for (;;) {
        std::function<void()> task;
        {
            std::unique_lock lock(mutex_);
            work_available_.wait(lock, [this] {
                return stopping_ || !queue_.empty();
            });
            if (stopping_ && queue_.empty()) return;
            task = std::move(queue_.front());
            queue_.pop_front();
            ++active_;
        }

        try {
            task();
        } catch (...) {
            // Submitted packaged tasks retain their exceptions in their futures.
        }

        {
            std::lock_guard lock(mutex_);
            --active_;
            if (queue_.empty() && active_ == 0) idle_.notify_all();
        }
    }
}

void JobSystem::shutdown() {
    {
        std::lock_guard lock(mutex_);
        stopping_ = true;
    }
    work_available_.notify_all();
    for (auto& worker : workers_) {
        if (worker.joinable()) worker.join();
    }
    workers_.clear();
}

void JobSystem::wait_idle() {
    std::unique_lock lock(mutex_);
    idle_.wait(lock, [this] { return queue_.empty() && active_ == 0; });
}

std::size_t JobSystem::worker_count() const noexcept {
    return workers_.size();
}

std::size_t JobSystem::pending_count() const {
    std::lock_guard lock(mutex_);
    return queue_.size();
}

bool JobSystem::is_stopping() const {
    std::lock_guard lock(mutex_);
    return stopping_;
}

} // namespace dream::core
