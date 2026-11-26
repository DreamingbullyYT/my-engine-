#pragma once

#include <chrono>
#include <cstddef>
#include <future>
#include <functional>
#include <mutex>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>

namespace dream::assets {

// Shares only currently-running requests. Completed entries are lazily pruned
// on the next request, so this registry does not become a completed-result cache.
// The caller chooses the key namespace (for example, include asset type and
// source identity when those distinguish otherwise-equal logical keys).
template <typename Result>
class AssetRequestCoordinator final {
public:
    using SharedResult = std::shared_future<Result>;

    AssetRequestCoordinator() = default;
    AssetRequestCoordinator(const AssetRequestCoordinator&) = delete;
    AssetRequestCoordinator& operator=(const AssetRequestCoordinator&) = delete;

    // The factory must return std::future<Result>. It is invoked at most once
    // for a key while that request remains pending. It must not re-enter this
    // coordinator. Exceptions from the factory are propagated to the caller.
    template <typename Factory>
    [[nodiscard]] SharedResult request(std::string key, Factory&& factory) {
        if (key.empty() || key.size() > 4096) {
            throw std::invalid_argument("Asset request key must contain 1 to 4096 bytes");
        }

        std::lock_guard lock(mutex_);
        prune_completed_locked();

        if (const auto found = requests_.find(key); found != requests_.end()) {
            return found->second;
        }

        using Future = std::invoke_result_t<std::decay_t<Factory>&>;
        static_assert(std::is_same_v<std::remove_cvref_t<Future>, std::future<Result>>,
                      "Request factory must return std::future<Result>");

        auto pending = std::invoke(std::forward<Factory>(factory));
        if (!pending.valid()) {
            throw std::invalid_argument("Asset request factory returned an invalid future");
        }

        auto shared = pending.share();
        requests_.emplace(std::move(key), shared);
        return shared;
    }

    // Removes completed requests without waiting. Existing callers retain
    // their shared_future and can still read the completed result.
    void prune_completed() {
        std::lock_guard lock(mutex_);
        prune_completed_locked();
    }

    [[nodiscard]] std::size_t in_flight_count() {
        std::lock_guard lock(mutex_);
        prune_completed_locked();
        return requests_.size();
    }

private:
    void prune_completed_locked() {
        for (auto current = requests_.begin(); current != requests_.end();) {
            if (current->second.wait_for(std::chrono::seconds(0)) ==
                std::future_status::ready) {
                current = requests_.erase(current);
            } else {
                ++current;
            }
        }
    }

    std::mutex mutex_;
    std::unordered_map<std::string, SharedResult> requests_;
};

} // namespace dream::assets
