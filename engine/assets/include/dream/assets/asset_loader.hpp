#pragma once

#include <dream/assets/asset_cache.hpp>
#include <dream/core/job_system.hpp>
#include <dream/core/resource_store.hpp>
#include <dream/platform/filesystem.hpp>

#include <cstddef>
#include <future>
#include <functional>
#include <optional>
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
    InvalidPath,
    InvalidKey,
    InvalidLimit,
    ReadFailed,
    DecodeFailed,
    StoreRejected
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
    std::size_t max_file_bytes = 64u * 1024u * 1024u)
    -> std::future<AssetLoadResult<core::Handle<Tag>>> {
    using Result = AssetLoadResult<core::Handle<Tag>>;
    using Decode = std::decay_t<Decoder>;
    static_assert(std::is_invocable_r_v<AssetDecodeResult<T>, Decode&, std::span<const std::byte>>,
                  "Decoder must return AssetDecodeResult<T> from a byte span");

    return jobs.submit(
        [&filesystem, &store, key = std::move(key),
         path = std::move(virtual_path),
         decode = Decode(std::forward<Decoder>(decoder)), max_file_bytes]() mutable -> Result {
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

            std::vector<std::byte> bytes;
            platform::FileError file_error = platform::FileError::None;
            if (!filesystem.read_file(path, bytes, max_file_bytes, file_error)) {
                return {AssetLoadStatus::ReadFailed, {}, file_error,
                        "Filesystem could not read the asset"};
            }

            auto decoded = std::invoke(decode, std::span<const std::byte>(bytes));
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
    std::size_t max_file_bytes = 64u * 1024u * 1024u)
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
         max_file_bytes]() mutable -> Result {
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

            if (const Handle existing = cache.find(key); existing) {
                return {AssetLoadStatus::CacheHit, existing, platform::FileError::None, {}};
            }

            std::vector<std::byte> bytes;
            platform::FileError file_error = platform::FileError::None;
            if (!filesystem.read_file(path, bytes, max_file_bytes, file_error)) {
                return {AssetLoadStatus::ReadFailed, {}, file_error,
                        "Filesystem could not read the asset"};
            }

            auto decoded = std::invoke(decode, std::span<const std::byte>(bytes));
            if (!decoded.value) {
                return {AssetLoadStatus::DecodeFailed, {}, platform::FileError::None,
                        std::move(decoded.message)};
            }

            const std::size_t estimated_bytes = std::invoke(estimate, std::as_const(*decoded.value));
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

} // namespace dream::assets
