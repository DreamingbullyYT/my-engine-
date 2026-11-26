#pragma once

#include <atomic>
#include <chrono>
#include <cstddef>
#include <future>
#include <functional>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>

namespace dream::assets {

// A cancellable view of a shared request. Cancelling this subscription changes
// only its result; other subscribers continue waiting for the same operation.
// The shared operation is intentionally not cancelled when one subscriber leaves.
template <typename Result>
class AssetRequestSubscription final {
public:
    AssetRequestSubscription() = default;

    void cancel() noexcept {
        if (cancelled_) cancelled_->store(true, std::memory_order_release);
    }

    [[nodiscard]] bool is_cancelled() const noexcept {
        return cancelled_ && cancelled_->load(std::memory_order_acquire);
    }

    [[nodiscard]] bool valid() const noexcept {
        return future_.valid() && static_cast<bool>(cancelled_) &&
            static_cast<bool>(cancelled_result_);
    }

    // Poll in short bounded intervals so cancellation is observed even while
    // another subscriber keeps the shared operation pending.
    [[nodiscard]] Result get() const {
        if (!valid()) throw std::future_error(std::future_errc::no_state);
        while (!is_cancelled()) {
            if (future_.wait_for(std::chrono::milliseconds(5)) ==
                std::future_status::ready) {
                break;
            }
        }
        if (is_cancelled()) return cancelled_result_();
        return future_.get();
    }

private:
    template <typename> friend class AssetRequestCoordinator;

    AssetRequestSubscription(
        std::shared_future<Result> future,
        std::function<Result()> cancelled_result)
        : future_(std::move(future)),
          cancelled_(std::make_shared<std::atomic_bool>(false)),
          cancelled_result_(std::move(cancelled_result)) {}

    std::shared_future<Result> future_;
    std::shared_ptr<std::atomic_bool> cancelled_;
    std::function<Result()> cancelled_result_;
};

// Shares only currently-running requests. Completed entries are lazily pruned
// on the next request, so this registry does not become a completed-result cache.
// The caller chooses the key namespace (for example, include asset type and
// source identity when those distinguish otherwise-equal logical keys).
template <typename Result>
class AssetRequestCoordinator final {
public:
    using SharedResult = std::shared_future<Result>;
    using Subscription = AssetRequestSubscription<Result>;

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

    // Creates a per-caller cancellable subscription to the shared result.
    // The cancellation factory must return the result representing a cancelled
    // subscriber. It is not invoked until this subscription's get() is called.
    template <typename Factory, typename CancelledResultFactory>
    [[nodiscard]] Subscription request_subscribed(
        std::string key, Factory&& factory,
        CancelledResultFactory&& cancelled_result_factory) {
        auto shared = request(std::move(key), std::forward<Factory>(factory));
        return Subscription(
            std::move(shared),
            std::function<Result()>(
                std::forward<CancelledResultFactory>(cancelled_result_factory)));
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
