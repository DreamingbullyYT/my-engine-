#pragma once

#include <dream/core/resource_store.hpp>

#include <cstddef>
#include <cstdint>
#include <algorithm>
#include <iterator>
#include <vector>
#include <functional>
#include <map>
#include <mutex>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

namespace dream::assets {

enum class AssetCachePutStatus {
    Inserted,
    InvalidKey,
    DuplicateKey,
    TooLarge
};

template <typename Handle>
struct AssetCachePutResult final {
    AssetCachePutStatus status{AssetCachePutStatus::InvalidKey};
    Handle handle{};

    [[nodiscard]] bool inserted() const noexcept {
        return status == AssetCachePutStatus::Inserted && static_cast<bool>(handle);
    }
};

// A byte-budgeted, least-recently-used owner for decoded CPU-side assets.
// The caller supplies each asset's estimated cost in bytes. This cache does not
// measure allocations, pin resources, perform I/O, or manage GPU residency.
// Access callbacks execute under the cache lock and must not re-enter this cache.
template <typename T, typename Tag>
class AssetCache final {
public:
    using Handle = core::Handle<Tag>;

    explicit AssetCache(std::size_t byte_budget) : byte_budget_(byte_budget) {}

    AssetCache(const AssetCache&) = delete;
    AssetCache& operator=(const AssetCache&) = delete;

    template <typename... Args>
    [[nodiscard]] AssetCachePutResult<Handle> put(
        std::string key, std::size_t estimated_bytes, Args&&... args) {
        std::lock_guard lock(mutex_);
        if (key.empty() || key.size() > 4096) {
            return {AssetCachePutStatus::InvalidKey, {}};
        }
        if (entries_.contains(key)) {
            return {AssetCachePutStatus::DuplicateKey, {}};
        }
        if (estimated_bytes > byte_budget_) {
            return {AssetCachePutStatus::TooLarge, {}};
        }

        // Construct and register first, so a constructor/allocation failure
        // cannot evict otherwise usable cache entries.
        auto handle = resources_.emplace(key, std::forward<Args>(args)...);
        if (!handle) {
            return {AssetCachePutStatus::InvalidKey, {}};
        }

        try {
            const auto [entry, inserted] =
                entries_.emplace(key, Entry{handle, estimated_bytes, next_stamp()});
            (void)entry;
            if (!inserted) {
                resources_.erase(key);
                return {AssetCachePutStatus::DuplicateKey, {}};
            }
        } catch (...) {
            resources_.erase(key);
            throw;
        }

        // The new entry has the newest LRU stamp and is therefore retained.
        evict_until_fits(estimated_bytes);
        used_bytes_ += estimated_bytes;
        return {AssetCachePutStatus::Inserted, handle};
    }

    [[nodiscard]] bool contains(std::string_view key) const {
        std::lock_guard lock(mutex_);
        return entries_.contains(std::string(key));
    }

    [[nodiscard]] Handle find(std::string_view key) const {
        std::lock_guard lock(mutex_);
        const auto found = entries_.find(std::string(key));
        return found == entries_.end() ? Handle{} : found->second.handle;
    }

    template <typename Callback>
    [[nodiscard]] bool with_resource(std::string_view key, Callback&& callback) {
        static_assert(std::is_invocable_r_v<void, Callback&, T&>,
                      "AssetCache callback must accept T& and return void");
        std::lock_guard lock(mutex_);
        const auto found = entries_.find(std::string(key));
        if (found == entries_.end()) return false;
        found->second.last_used = next_stamp();
        return resources_.with_resource(found->second.handle,
                                        std::forward<Callback>(callback));
    }

    [[nodiscard]] bool erase(std::string_view key) {
        std::lock_guard lock(mutex_);
        const auto found = entries_.find(std::string(key));
        if (found == entries_.end()) return false;
        if (!resources_.erase(std::string(key))) return false;
        used_bytes_ -= found->second.estimated_bytes;
        entries_.erase(found);
        return true;
    }

    void set_byte_budget(std::size_t new_budget) {
        std::lock_guard lock(mutex_);
        byte_budget_ = new_budget;
        evict_until_fits(0);
    }

    [[nodiscard]] std::size_t byte_budget() const {
        std::lock_guard lock(mutex_);
        return byte_budget_;
    }

    [[nodiscard]] std::size_t used_bytes() const {
        std::lock_guard lock(mutex_);
        return used_bytes_;
    }

    [[nodiscard]] std::size_t size() const {
        std::lock_guard lock(mutex_);
        return entries_.size();
    }

private:
    struct Entry final {
        Handle handle;
        std::size_t estimated_bytes;
        std::uint64_t last_used;
    };

    [[nodiscard]] std::uint64_t next_stamp() {
        // On wrap, renormalize the small metadata set while preserving LRU order.
        if (clock_ == UINT64_MAX) {
            std::vector<Entry*> ordered;
            ordered.reserve(entries_.size());
            for (auto& [key, entry] : entries_) {
                (void)key;
                ordered.push_back(&entry);
            }
            std::stable_sort(ordered.begin(), ordered.end(),
                [](const Entry* left, const Entry* right) {
                    return left->last_used < right->last_used;
                });
            std::uint64_t stamp = 1;
            for (auto* entry : ordered) entry->last_used = stamp++;
            clock_ = stamp - 1;
        }
        return ++clock_;
    }

    void evict_until_fits(std::size_t incoming_bytes) {
        while (used_bytes_ > byte_budget_ - incoming_bytes && !entries_.empty()) {
            auto oldest = entries_.begin();
            for (auto current = std::next(entries_.begin()); current != entries_.end(); ++current) {
                if (current->second.last_used < oldest->second.last_used) oldest = current;
            }
            const auto key = oldest->first;
            const auto bytes = oldest->second.estimated_bytes;
            if (!resources_.erase(key)) break;
            entries_.erase(oldest);
            used_bytes_ -= bytes;
        }
    }

    mutable std::mutex mutex_;
    core::ResourceStore<T, Tag> resources_;
    std::map<std::string, Entry, std::less<>> entries_;
    std::size_t byte_budget_{0};
    std::size_t used_bytes_{0};
    std::uint64_t clock_{0};
};

} // namespace dream::assets
