#pragma once

#include <dream/core/resource_registry.hpp>

#include <cstddef>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>

namespace dream::core {

// A typed, case-sensitive key-to-resource layer. It does not perform file I/O,
// decoding or asynchronous loading; those policies belong to higher-level systems.
template <typename T, typename Tag>
class ResourceStore final {
public:
    using handle_type = Handle<Tag>;

    ResourceStore() = default;
    ResourceStore(const ResourceStore&) = delete;
    ResourceStore& operator=(const ResourceStore&) = delete;

    template <typename... Args>
    [[nodiscard]] handle_type emplace(std::string key, Args&&... args) {
        if (key.empty() || key.size() > 4096) return {};

        std::unique_lock lock(mutex_);
        if (entries_.find(key) != entries_.end()) return {};

        // Insert an inaccessible placeholder first. If resource construction or
        // registry allocation fails, erase it; this avoids orphaned resources if
        // allocating the key map entry were to fail after resource creation.
        const auto [entry, inserted] = entries_.emplace(std::move(key), handle_type{});
        if (!inserted) return {};

        try {
            const auto handle = resources_.emplace(std::forward<Args>(args)...);
            if (!handle) {
                entries_.erase(entry);
                return {};
            }
            entry->second = handle;
            return handle;
        } catch (...) {
            entries_.erase(entry);
            throw;
        }
    }

    // Returns an invalid handle when the key is absent.
    [[nodiscard]] handle_type find(std::string_view key) const {
        if (key.empty()) return {};
        std::shared_lock lock(mutex_);
        const auto entry = entries_.find(std::string(key));
        return entry == entries_.end() ? handle_type{} : entry->second;
    }

    [[nodiscard]] bool contains(std::string_view key) const {
        return static_cast<bool>(find(key));
    }

    [[nodiscard]] bool erase(std::string_view key) {
        if (key.empty()) return false;
        std::unique_lock lock(mutex_);
        const auto entry = entries_.find(std::string(key));
        if (entry == entries_.end()) return false;

        // Keep the key if destruction fails, so the mapping remains consistent.
        if (!resources_.destroy(entry->second)) return false;
        entries_.erase(entry);
        return true;
    }

    // Resource callbacks must not re-enter this same store; the shared lock
    // protects the key mapping and the underlying registry lifetime.
    [[nodiscard]] bool with_resource(handle_type handle, auto&& function) const {
        std::shared_lock lock(mutex_);
        return resources_.with_resource(handle, std::forward<decltype(function)>(function));
    }

    [[nodiscard]] bool with_resource(
        std::string_view key, auto&& function) const {
        if (key.empty()) return false;
        std::shared_lock lock(mutex_);
        const auto entry = entries_.find(std::string(key));
        if (entry == entries_.end()) return false;
        return resources_.with_resource(
            entry->second, std::forward<decltype(function)>(function));
    }

    [[nodiscard]] std::size_t size() const {
        std::shared_lock lock(mutex_);
        return entries_.size();
    }

private:
    // Declared before entries_: entries_ is destroyed first, then resources_.
    mutable std::shared_mutex mutex_;
    ResourceRegistry<T, Tag> resources_;
    std::unordered_map<std::string, handle_type> entries_;
};

} // namespace dream::core
