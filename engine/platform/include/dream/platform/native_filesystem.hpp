#pragma once
#include <dream/platform/filesystem.hpp>
#include <string>
#if defined(__unix__) || defined(__APPLE__) || defined(__ANDROID__)
namespace dream::platform {
class NativeFileSystem final : public IFileSystem {
public:
    explicit NativeFileSystem(std::string root_directory);
    ~NativeFileSystem() override;
    NativeFileSystem(const NativeFileSystem&) = delete;
    NativeFileSystem& operator=(const NativeFileSystem&) = delete;
    NativeFileSystem(NativeFileSystem&&) = delete;
    NativeFileSystem& operator=(NativeFileSystem&&) = delete;
    [[nodiscard]] bool valid() const noexcept;
    [[nodiscard]] const std::string& initialization_error() const noexcept;
    [[nodiscard]] bool read_file(std::string_view, std::vector<std::byte>&,
        std::size_t, FileError&) const override;
    [[nodiscard]] bool write_file(std::string_view, std::span<const std::byte>,
        FileError&) override;
    [[nodiscard]] bool exists(std::string_view, bool&, FileError&) const override;
    [[nodiscard]] bool create_directories(std::string_view, FileError&) override;
    [[nodiscard]] bool remove_file(std::string_view, FileError&) override;
    [[nodiscard]] bool list_directory(std::string_view,
        std::vector<DirectoryEntry>&, FileError&) const override;
private:
    int root_fd_{-1};
    std::string initialization_error_;
};
} // namespace dream::platform
#endif
