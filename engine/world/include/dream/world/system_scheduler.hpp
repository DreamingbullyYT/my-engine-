#pragma once

#include <dream/world/world.hpp>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <utility>
#include <vector>

namespace dream::world {

struct SystemTag;
using SystemHandle = core::Handle<SystemTag>;

enum class SystemPhase : std::uint8_t {
    PreUpdate,
    Update,
    PostUpdate
};

enum class SystemAddError : std::uint8_t {
    None,
    InvalidName,
    InvalidPhase,
    EmptyCallback,
    CapacityExhausted
};

enum class SystemUpdateError : std::uint8_t {
    None,
    InvalidDeltaTime,
    AlreadyUpdating
};

struct SystemContext final {
    float delta_seconds{0.0F};
    std::uint64_t frame_index{0};
};

struct SystemAddResult final {
    SystemHandle handle{};
    SystemAddError error{SystemAddError::None};

    [[nodiscard]] explicit operator bool() const noexcept {
        return static_cast<bool>(handle) && error == SystemAddError::None;
    }
};

struct SystemFailure final {
    SystemHandle system{};
    std::string name;
    std::string message;
};

struct SystemUpdateReport final {
    SystemUpdateError error{SystemUpdateError::None};
    std::uint64_t frame_index{0};
    std::size_t executed{0};
    std::size_t skipped{0};
    std::vector<SystemFailure> failures;
};

// Runs scene systems in deterministic phase / priority / registration order.
// Higher priorities run first within a phase. A tick snapshots registrations,
// then releases the scheduler lock before callbacks execute, so callbacks may
// add or remove systems. Additions become visible on the next tick; a system
// removed before its turn is skipped. Removing a running callback cannot stop it.
class SystemScheduler final {
public:
    using Callback = std::function<void(World&, const SystemContext&)>;

    SystemScheduler() = default;
    SystemScheduler(const SystemScheduler&) = delete;
    SystemScheduler& operator=(const SystemScheduler&) = delete;

    [[nodiscard]] SystemAddResult add_system(
        std::string name, SystemPhase phase, std::int32_t priority, Callback callback) {
        if (name.empty() || name.size() > 128)
            return {{}, SystemAddError::InvalidName};
        if (!valid_phase(phase))
            return {{}, SystemAddError::InvalidPhase};
        if (!callback)
            return {{}, SystemAddError::EmptyCallback};

        std::unique_lock lock(mutex_);
        const auto handle = handles_.allocate();
        if (!handle) return {{}, SystemAddError::CapacityExhausted};

        try {
            auto entry = std::make_shared<Entry>(
                handle, std::move(name), phase, priority, next_order_++,
                std::move(callback));
            systems_.emplace(handle.index(), std::move(entry));
        } catch (...) {
            (void)handles_.release(handle);
            throw;
        }
        return {handle, SystemAddError::None};
    }

    [[nodiscard]] bool remove_system(SystemHandle handle) {
        if (!handle) return false;
        std::unique_lock lock(mutex_);
        const auto found = systems_.find(handle.index());
        if (found == systems_.end() || found->second->handle != handle) return false;
        if (!handles_.release(handle)) return false;
        found->second->active.store(false, std::memory_order_release);
        systems_.erase(found);
        return true;
    }

    [[nodiscard]] SystemUpdateReport update(World& world, float delta_seconds) {
        SystemUpdateReport report;
        if (!std::isfinite(delta_seconds) || delta_seconds < 0.0F) {
            report.error = SystemUpdateError::InvalidDeltaTime;
            return report;
        }

        if (updating_.test_and_set(std::memory_order_acquire)) {
            report.error = SystemUpdateError::AlreadyUpdating;
            return report;
        }
        struct UpdateGuard final {
            std::atomic_flag& flag;
            ~UpdateGuard() { flag.clear(std::memory_order_release); }
        } update_guard{updating_};

        std::vector<std::shared_ptr<Entry>> snapshot;
        {
            std::unique_lock lock(mutex_);
            if (frame_index_ != UINT64_MAX) ++frame_index_;
            report.frame_index = frame_index_;
            snapshot.reserve(systems_.size());
            for (const auto& [index, entry] : systems_) {
                (void)index;
                snapshot.push_back(entry);
            }
        }

        std::stable_sort(snapshot.begin(), snapshot.end(),
            [](const auto& left, const auto& right) {
                if (left->phase != right->phase)
                    return left->phase < right->phase;
                if (left->priority != right->priority)
                    return left->priority > right->priority;
                return left->order < right->order;
            });

        const SystemContext context{delta_seconds, report.frame_index};
        for (const auto& entry : snapshot) {
            if (!entry->active.load(std::memory_order_acquire)) {
                ++report.skipped;
                continue;
            }

            ++report.executed;
            try {
                entry->callback(world, context);
            } catch (const std::exception& error) {
                report.failures.push_back({entry->handle, entry->name, error.what()});
            } catch (...) {
                report.failures.push_back({
                    entry->handle, entry->name, "Unknown exception"});
            }
        }
        return report;
    }

    [[nodiscard]] std::size_t system_count() const {
        std::shared_lock lock(mutex_);
        return systems_.size();
    }

    [[nodiscard]] std::uint64_t frame_index() const {
        std::shared_lock lock(mutex_);
        return frame_index_;
    }

private:
    struct Entry final {
        Entry(SystemHandle system_handle, std::string system_name,
              SystemPhase system_phase, std::int32_t system_priority,
              std::uint64_t registration_order, Callback system_callback)
            : handle(system_handle), name(std::move(system_name)),
              phase(system_phase), priority(system_priority),
              order(registration_order), callback(std::move(system_callback)) {}

        SystemHandle handle;
        std::string name;
        SystemPhase phase;
        std::int32_t priority;
        std::uint64_t order;
        Callback callback;
        std::atomic_bool active{true};
    };

    [[nodiscard]] static bool valid_phase(SystemPhase phase) noexcept {
        switch (phase) {
        case SystemPhase::PreUpdate:
        case SystemPhase::Update:
        case SystemPhase::PostUpdate:
            return true;
        }
        return false;
    }

    mutable std::shared_mutex mutex_;
    std::atomic_flag updating_ = ATOMIC_FLAG_INIT;
    core::HandlePool<SystemTag> handles_;
    std::map<std::uint32_t, std::shared_ptr<Entry>> systems_;
    std::uint64_t next_order_{0};
    std::uint64_t frame_index_{0};
};

} // namespace dream::world
