#pragma once

#include <dream/core/handle.hpp>

#include <cstddef>
#include <functional>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <type_traits>
#include <utility>
#include <vector>

namespace dream::core {

// Owns resources and their identities. Access is scoped through with_resource()
// so destruction cannot race an active callback.
template <typename T, typename Tag>
class ResourceRegistry final {
public:
    using handle_type = Handle<Tag>;

    ResourceRegistry() = default;
    ResourceRegistry(const ResourceRegistry&) = delete;
    ResourceRegistry& operator=(const ResourceRegistry&) = delete;

    template <typename... Args>
    [[nodiscard]] handle_type emplace(Args&&... args) {
        auto resource = std::make_unique<T>(std::forward<Args>(args)...);
        std::unique_lock lock(mutex_);

        const auto handle = handles_.allocate();
        if (!handle) return {};

        try {
            if (slots_.size() <= handle.index()) {
                slots_.resize(static_cast<std::size_t>(handle.index()) + 1);
            }
            slots_[handle.index()] = std::move(resource);
            ++active_count_;
        } catch (...) {
            (void)handles_.release(handle);
            throw;
        }
        return handle;
    }

    // Invalidates the identity while holding the registry lock, then destroys
    // the object before another registry operation can reuse or access the slot.
    [[nodiscard]] bool destroy(handle_type handle) {
        if (!handle) return false;
        std::unique_lock lock(mutex_);
        if (!handles_.is_alive(handle) || handle.index() >= slots_.size() ||
            !slots_[handle.index()]) {
            return false;
        }

        // HandlePool::release() may allocate while growing its free list. Do it
        // before destroying the resource so an allocation failure leaves the
        // live registry entry intact. The exclusive registry lock prevents any
        // other operation from reusing or observing the slot until reset ends.
        if (!handles_.release(handle)) return false;
        slots_[handle.index()].reset();
        --active_count_;
        return true;
    }

    [[nodiscard]] bool contains(handle_type handle) const {
        if (!handle) return false;
        std::shared_lock lock(mutex_);
        return handles_.is_alive(handle) && handle.index() < slots_.size() &&
               static_cast<bool>(slots_[handle.index()]);
    }

    // The callback runs while the registry holds a shared lock. It must not
    // call emplace(), destroy(), or otherwise re-enter this same registry.
    // Returning references is forbidden to prevent resource references escaping
    // the lifetime guard.
    template <typename Function>
    [[nodiscard]] bool with_resource(handle_type handle, Function&& function) const {
        static_assert(std::is_invocable_v<Function, T&> &&
                      std::is_same_v<std::invoke_result_t<Function, T&>, void>,
                      "Resource callback must accept T& and return exactly void");
        if (!handle) return false;

        std::shared_lock lock(mutex_);
        if (!handles_.is_alive(handle) || handle.index() >= slots_.size() ||
            !slots_[handle.index()]) {
            return false;
        }

        std::invoke(std::forward<Function>(function), *slots_[handle.index()]);
        return true;
    }

    [[nodiscard]] std::size_t size() const {
        std::shared_lock lock(mutex_);
        return active_count_;
    }

private:
    mutable std::shared_mutex mutex_;
    HandlePool<Tag> handles_;
    std::vector<std::unique_ptr<T>> slots_;
    std::size_t active_count_{0};
};

} // namespace dream::core
