#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <span>
#include <vector>

namespace dream::platform {

enum class FileError : std::uint8_t {
    None,
    InvalidPath,
    NotFound,
    AlreadyExists,
    PermissionDenied,
    NotDirectory,
    IsDirectory,
    Io,
    Unsupported,
    TooLarge
};

struct FileInfo final {
    std::uint64_t size_bytes{0};
    bool is_directory{false};
    bool is_regular_file{false};
};

struct DirectoryEntry final {
    std::string name;
    FileInfo info{};
};

// Paths are UTF-8 virtual paths relative to a backend-defined root.
// They must use '/' separators and may not escape that root.
// Implementations should leave output unchanged when a read/list operation fails.
// read_file must check file size before allocating and reject files larger than max_bytes.
[[nodiscard]] bool is_valid_virtual_path(std::string_view path) noexcept;

class IFileSystem {
public:
    virtual ~IFileSystem() = default;

    [[nodiscard]] virtual bool read_file(
        std::string_view path, std::vector<std::byte>& output,
        std::size_t max_bytes, FileError& error) const = 0;
    [[nodiscard]] virtual bool write_file(
        std::string_view path, std::span<const std::byte> data,
        FileError& error) = 0;
    [[nodiscard]] virtual bool exists(
        std::string_view path, bool& result, FileError& error) const = 0;
    [[nodiscard]] virtual bool create_directories(
        std::string_view path, FileError& error) = 0;
    [[nodiscard]] virtual bool remove_file(
        std::string_view path, FileError& error) = 0;
    [[nodiscard]] virtual bool list_directory(
        std::string_view path, std::vector<DirectoryEntry>& output,
        FileError& error) const = 0;
};

} // namespace dream::platform
