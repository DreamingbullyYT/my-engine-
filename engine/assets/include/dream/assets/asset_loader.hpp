#pragma once

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
        return status == AssetLoadStatus::Loaded && static_cast<bool>(handle);
    }
};

// The filesystem and store are borrowed. They, and the JobSystem, must outlive
// the returned future's task. Decoder must be safe to invoke on a worker thread.
// This first layer deliberately has no global cache, dependency graph or GPU upload.
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

} // namespace dream::assets
