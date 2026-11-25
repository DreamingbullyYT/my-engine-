#pragma once

#include <dream/dmod/manifest.hpp>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace dream::dmod {

struct PackageLimits final {
    std::uint64_t max_archive_bytes{256ull * 1024ull * 1024ull};
    std::uint32_t max_entries{4096};
    std::uint64_t max_entry_uncompressed_bytes{128ull * 1024ull * 1024ull};
    std::uint64_t max_total_uncompressed_bytes{512ull * 1024ull * 1024ull};
    std::uint64_t max_compression_ratio{200};
    std::uint64_t max_manifest_bytes{64ull * 1024ull};
};

struct PackageEntry final {
    std::string path;
    std::uint64_t compressed_size{};
    std::uint64_t uncompressed_size{};
    bool directory{false};
};

struct PackageFileResult final {
    std::vector<std::byte> bytes;
    std::string error;

    [[nodiscard]] bool success() const noexcept { return error.empty(); }
};

class PackageReader final {
public:
    // The caller must keep archive_bytes alive and unchanged for this reader's lifetime.
    // The reader never extracts package entries to the filesystem.
    // A single PackageReader instance is not safe for concurrent read_file() calls.
    [[nodiscard]] static std::unique_ptr<PackageReader> open(
        std::span<const std::byte> archive_bytes,
        std::vector<std::string>& errors,
        PackageLimits limits = {});

    ~PackageReader();

    PackageReader(const PackageReader&) = delete;
    PackageReader& operator=(const PackageReader&) = delete;
    PackageReader(PackageReader&&) = delete;
    PackageReader& operator=(PackageReader&&) = delete;

    [[nodiscard]] const Manifest& manifest() const noexcept;
    [[nodiscard]] const std::vector<PackageEntry>& entries() const noexcept;
    [[nodiscard]] PackageFileResult read_file(std::string_view path) const;

private:
    struct Impl;
    explicit PackageReader(std::unique_ptr<Impl> impl);

    std::unique_ptr<Impl> impl_;
};

} // namespace dream::dmod
