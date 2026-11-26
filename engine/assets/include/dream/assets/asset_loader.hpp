#pragma once

#include <dream/assets/asset_cache.hpp>
#include <dream/assets/asset_request_coordinator.hpp>
#include <dream/core/job_system.hpp>
#include <dream/core/resource_store.hpp>
#include <dream/platform/filesystem.hpp>

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <future>
#include <functional>
#include <mutex>
#include <optional>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace dream::assets {

enum class AssetLoadStatus {
    Loaded,
    CacheHit,
    CacheRejected,
    Cancelled,
    InvalidPath,
    InvalidKey,
    InvalidLimit,
    ReadFailed,
    DecodeFailed,
    StoreRejected
};

class AssetCancellationToken final {
public:
    AssetCancellationToken() = default;
    [[nodiscard]] bool is_cancelled() const noexcept {
        return state_ && state_->load(std::memory_order_acquire);
    }
private:
    explicit AssetCancellationToken(std::shared_ptr<std::atomic_bool> state)
        : state_(std::move(state)) {}
    AssetCancellationToken(
        std::shared_ptr<std::atomic_bool> state,
        std::shared_ptr<void> lifetime_pin)
        : state_(std::move(state)), lifetime_pin_(std::move(lifetime_pin)) {}
    std::shared_ptr<std::atomic_bool> state_;
    // Keeps a scoped load lease alive for the entire asynchronous task.
    std::shared_ptr<void> lifetime_pin_;
    friend class AssetCancellationSource;
    friend class AssetLoadScope;
};

class AssetCancellationSource final {
public:
    AssetCancellationSource() : state_(std::make_shared<std::atomic_bool>(false)) {}
    [[nodiscard]] AssetCancellationToken token() const noexcept {
        return AssetCancellationToken(state_);
    }
    void cancel() noexcept { state_->store(true, std::memory_order_release); }
    [[nodiscard]] bool is_cancelled() const noexcept {
        return state_->load(std::memory_order_acquire);
    }
private:
    std::shared_ptr<std::atomic_bool> state_;
};

// Tracks asynchronous tasks that borrow engine services. The task must receive
// a token returned by acquire_token(); that token pins the scope until the task
// releases its capture. shutdown_and_wait() closes admission, requests
// cooperative cancellation, then waits until every scoped task has exited.
//
// Declare the scope after the JobSystem, filesystem, store/cache and decoder
// dependencies it protects, so the scope is destroyed first. Do not call
// shutdown_and_wait() from one of the scope's own worker tasks. A blocking
// filesystem read or decoder cannot be interrupted; shutdown waits for it.
class AssetLoadScope final {
private:
    struct State final {
        std::shared_ptr<std::atomic_bool> cancelled{
            std::make_shared<std::atomic_bool>(false)};
        mutable std::mutex mutex;
        std::condition_variable idle;
        std::size_t active{0};
        bool shutting_down{false};
    };

    struct Lease final {
        explicit Lease(std::shared_ptr<State> owner) : state(std::move(owner)) {}
        ~Lease() {
            if (!registered || !state) return;
            std::lock_guard lock(state->mutex);
            if (state->active > 0) --state->active;
            if (state->active == 0) state->idle.notify_all();
        }
        std::shared_ptr<State> state;
        bool registered{false};
    };

public:
    AssetLoadScope() : state_(std::make_shared<State>()) {}
    AssetLoadScope(const AssetLoadScope&) = delete;
    AssetLoadScope& operator=(const AssetLoadScope&) = delete;
    AssetLoadScope(AssetLoadScope&&) = delete;
    AssetLoadScope& operator=(AssetLoadScope&&) = delete;

    ~AssetLoadScope() { shutdown_and_wait(); }

    // One acquired token represents one scoped async operation. Move it into
    // load_async()/load_async_cached(); copies share the same lifetime lease.
    [[nodiscard]] AssetCancellationToken acquire_token() {
        auto lease = std::make_shared<Lease>(state_);
        {
            std::lock_guard lock(state_->mutex);
            if (state_->shutting_down) {
                return AssetCancellationToken(state_->cancelled);
            }
            ++state_->active;
            lease->registered = true;
        }
        return AssetCancellationToken(state_->cancelled, std::move(lease));
    }

    // Idempotent. Waits for queued/running tasks holding acquired tokens.
    void shutdown_and_wait() noexcept {
        std::unique_lock lock(state_->mutex);
        state_->shutting_down = true;
        state_->cancelled->store(true, std::memory_order_release);
        state_->idle.wait(lock, [this] { return state_->active == 0; });
    }

    [[nodiscard]] bool is_cancelled() const noexcept {
        return state_->cancelled->load(std::memory_order_acquire);
    }

    [[nodiscard]] bool is_shutting_down() const noexcept {
        std::lock_guard lock(state_->mutex);
        return state_->shutting_down;
    }

    [[nodiscard]] std::size_t active_count() const noexcept {
        std::lock_guard lock(state_->mutex);
        return state_->active;
    }

private:
    std::shared_ptr<State> state_;
};

template <typename T>
struct AssetDecodeResult final {
    std::optional<T> value;
    std::string message;

    [[nodiscard]] static AssetDecodeResult success(T resource) {
        return {std::optional<T>(std::move(resource)), {}};
    }

    [[nodiscard]] static AssetDecodeResult failure(std::string reason) {
        return {std::nullopt, std::move(reason)};
    }
};

template <typename Handle>
struct AssetLoadResult final {
    AssetLoadStatus status{AssetLoadStatus::ReadFailed};
    Handle handle{};
    platform::FileError file_error{platform::FileError::None};
    std::string message;

    [[nodiscard]] bool loaded() const noexcept {
        return (status == AssetLoadStatus::Loaded || status == AssetLoadStatus::CacheHit) &&
               static_cast<bool>(handle);
    }
};

// The filesystem, store and JobSystem are borrowed. They must outlive the task.
// Decoder must be safe to invoke on a worker thread. This uncached path keeps
// ResourceStore ownership independent from the optional AssetCache layer.
template <typename T, typename Tag, typename Decoder>
[[nodiscard]] auto load_async(
    core::JobSystem& jobs,
    const platform::IFileSystem& filesystem,
    core::ResourceStore<T, Tag>& store,
    std::string key,
    std::string virtual_path,
    Decoder&& decoder,
    std::size_t max_file_bytes = 64u * 1024u * 1024u,
    AssetCancellationToken cancellation = {})
    -> std::future<AssetLoadResult<core::Handle<Tag>>> {
    using Result = AssetLoadResult<core::Handle<Tag>>;
    using Decode = std::decay_t<Decoder>;
    static_assert(std::is_invocable_r_v<AssetDecodeResult<T>, Decode&, std::span<const std::byte>>,
                  "Decoder must return AssetDecodeResult<T> from a byte span");

    return jobs.submit(
        [&filesystem, &store, key = std::move(key),
         path = std::move(virtual_path),
         decode = Decode(std::forward<Decoder>(decoder)), max_file_bytes,
         cancellation = std::move(cancellation)]() mutable -> Result {
            if (!platform::is_valid_virtual_path(path)) {
                return {AssetLoadStatus::InvalidPath, {}, platform::FileError::InvalidPath,
                        "Asset path is not a valid virtual path"};
            }
            if (key.empty() || key.size() > 4096) {
                return {AssetLoadStatus::InvalidKey, {}, platform::FileError::None,
                        "Asset key must contain 1 to 4096 bytes"};
            }
            if (max_file_bytes == 0) {
                return {AssetLoadStatus::InvalidLimit, {}, platform::FileError::None,
                        "Maximum asset file size must be greater than zero"};
            }

            if (cancellation.is_cancelled()) {
                return {AssetLoadStatus::Cancelled, {}, platform::FileError::None,
                        "Asset load was cancelled"};
            }
            std::vector<std::byte> bytes;
            platform::FileError file_error = platform::FileError::None;
            const bool read_succeeded = filesystem.read_file(path, bytes, max_file_bytes, file_error);
            if (cancellation.is_cancelled()) {
                return {AssetLoadStatus::Cancelled, {}, platform::FileError::None,
                        "Asset load was cancelled"};
            }
            if (!read_succeeded) {
                return {AssetLoadStatus::ReadFailed, {}, file_error,
                        "Filesystem could not read the asset"};
            }

            auto decoded = std::invoke(decode, std::span<const std::byte>(bytes));
            if (cancellation.is_cancelled()) {
                return {AssetLoadStatus::Cancelled, {}, platform::FileError::None,
                        "Asset load was cancelled"};
            }
            if (!decoded.value) {
                return {AssetLoadStatus::DecodeFailed, {}, platform::FileError::None,
                        std::move(decoded.message)};
            }

            auto handle = store.emplace(std::move(key), std::move(*decoded.value));
            if (!handle) {
                return {AssetLoadStatus::StoreRejected, {}, platform::FileError::None,
                        "ResourceStore rejected the key or resource"};
            }
            return {AssetLoadStatus::Loaded, handle, platform::FileError::None, {}};
        });
}

// Cached variant. The cache and all borrowed dependencies must outlive the task.
// estimate_bytes runs on the worker after decoding and must return a conservative
// CPU-memory estimate; it does not measure allocations or GPU residency.
template <typename T, typename Tag, typename Decoder, typename SizeEstimator>
[[nodiscard]] auto load_async_cached(
    core::JobSystem& jobs,
    const platform::IFileSystem& filesystem,
    AssetCache<T, Tag>& cache,
    std::string key,
    std::string virtual_path,
    Decoder&& decoder,
    SizeEstimator&& estimate_bytes,
    std::size_t max_file_bytes = 64u * 1024u * 1024u,
    AssetCancellationToken cancellation = {})
    -> std::future<AssetLoadResult<core::Handle<Tag>>> {
    using Result = AssetLoadResult<core::Handle<Tag>>;
    using Handle = core::Handle<Tag>;
    using Decode = std::decay_t<Decoder>;
    using Estimate = std::decay_t<SizeEstimator>;
    static_assert(std::is_invocable_r_v<AssetDecodeResult<T>, Decode&, std::span<const std::byte>>,
                  "Decoder must return AssetDecodeResult<T> from a byte span");
    static_assert(std::is_invocable_r_v<std::size_t, Estimate&, const T&>,
                  "Size estimator must return size_t from const T&");

    return jobs.submit(
        [&filesystem, &cache, key = std::move(key), path = std::move(virtual_path),
         decode = Decode(std::forward<Decoder>(decoder)),
         estimate = Estimate(std::forward<SizeEstimator>(estimate_bytes)),
         max_file_bytes, cancellation = std::move(cancellation)]() mutable -> Result {
            if (!platform::is_valid_virtual_path(path)) {
                return {AssetLoadStatus::InvalidPath, {}, platform::FileError::InvalidPath,
                        "Asset path is not a valid virtual path"};
            }
            if (key.empty() || key.size() > 4096) {
                return {AssetLoadStatus::InvalidKey, {}, platform::FileError::None,
                        "Asset key must contain 1 to 4096 bytes"};
            }
            if (max_file_bytes == 0) {
                return {AssetLoadStatus::InvalidLimit, {}, platform::FileError::None,
                        "Maximum asset file size must be greater than zero"};
            }

            if (cancellation.is_cancelled()) {
                return {AssetLoadStatus::Cancelled, {}, platform::FileError::None,
                        "Asset load was cancelled"};
            }
            if (const Handle existing = cache.find(key); existing) {
                return {AssetLoadStatus::CacheHit, existing, platform::FileError::None, {}};
            }

            std::vector<std::byte> bytes;
            platform::FileError file_error = platform::FileError::None;
            const bool read_succeeded = filesystem.read_file(path, bytes, max_file_bytes, file_error);
            if (cancellation.is_cancelled()) {
                return {AssetLoadStatus::Cancelled, {}, platform::FileError::None,
                        "Asset load was cancelled"};
            }
            if (!read_succeeded) {
                return {AssetLoadStatus::ReadFailed, {}, file_error,
                        "Filesystem could not read the asset"};
            }

            auto decoded = std::invoke(decode, std::span<const std::byte>(bytes));
            if (cancellation.is_cancelled()) {
                return {AssetLoadStatus::Cancelled, {}, platform::FileError::None,
                        "Asset load was cancelled"};
            }
            if (!decoded.value) {
                return {AssetLoadStatus::DecodeFailed, {}, platform::FileError::None,
                        std::move(decoded.message)};
            }

            const std::size_t estimated_bytes = std::invoke(estimate, std::as_const(*decoded.value));
            if (cancellation.is_cancelled()) {
                return {AssetLoadStatus::Cancelled, {}, platform::FileError::None,
                        "Asset load was cancelled"};
            }
            auto inserted = cache.put(key, estimated_bytes, std::move(*decoded.value));
            if (!inserted.inserted()) {
                std::string message = "AssetCache rejected the decoded resource";
                if (inserted.status == AssetCachePutStatus::DuplicateKey) {
                    message = "AssetCache already contains this key";
                } else if (inserted.status == AssetCachePutStatus::TooLarge) {
                    message = "Decoded asset exceeds the AssetCache byte budget";
                } else if (inserted.status == AssetCachePutStatus::InvalidKey) {
                    message = "AssetCache rejected the asset key";
                }
                return {AssetLoadStatus::CacheRejected, {}, platform::FileError::None,
                        std::move(message)};
            }
            return {AssetLoadStatus::Loaded, inserted.handle, platform::FileError::None, {}};
        });
}


/*
 * Coordinated variants deduplicate requests that share request_id while the
 * first operation is still pending. request_id is a caller-defined identity;
 * include source/type/version information when it distinguishes requests.
 *
 * The first caller owns the operation's cancellation token and decoder. Later
 * callers with the same pending request_id receive its shared result; their
 * factory arguments and cancellation tokens are not used.
 */
template <typename T, typename Tag, typename Decoder>
[[nodiscard]] auto load_async_coordinated(
    AssetRequestCoordinator<AssetLoadResult<core::Handle<Tag>>>& coordinator,
    std::string request_id,
    core::JobSystem& jobs,
    const platform::IFileSystem& filesystem,
    core::ResourceStore<T, Tag>& store,
    std::string key,
    std::string virtual_path,
    Decoder&& decoder,
    std::size_t max_file_bytes = 64u * 1024u * 1024u,
    AssetCancellationToken cancellation = {})
    -> AssetRequestCoordinator<AssetLoadResult<core::Handle<Tag>>>::SharedResult {
    return coordinator.request(std::move(request_id), [&]() mutable {
        return load_async<T, Tag>(
            jobs, filesystem, store, std::move(key), std::move(virtual_path),
            std::forward<Decoder>(decoder), max_file_bytes, std::move(cancellation));
    });
}

template <typename T, typename Tag, typename Decoder, typename SizeEstimator>
[[nodiscard]] auto load_async_cached_coordinated(
    AssetRequestCoordinator<AssetLoadResult<core::Handle<Tag>>>& coordinator,
    std::string request_id,
    core::JobSystem& jobs,
    const platform::IFileSystem& filesystem,
    AssetCache<T, Tag>& cache,
    std::string key,
    std::string virtual_path,
    Decoder&& decoder,
    SizeEstimator&& estimate_bytes,
    std::size_t max_file_bytes = 64u * 1024u * 1024u,
    AssetCancellationToken cancellation = {})
    -> AssetRequestCoordinator<AssetLoadResult<core::Handle<Tag>>>::SharedResult {
    return coordinator.request(std::move(request_id), [&]() mutable {
        return load_async_cached<T, Tag>(
            jobs, filesystem, cache, std::move(key), std::move(virtual_path),
            std::forward<Decoder>(decoder), std::forward<SizeEstimator>(estimate_bytes),
            max_file_bytes, std::move(cancellation));
    });
}

} // namespace dream::assets
