#pragma once
#if defined(__ANDROID__)
#include <dream/platform/filesystem.hpp>
#include <android/asset_manager.h>
namespace dream::platform {
// Read-only APK assets. The borrowed AAssetManager must outlive this adapter and its reads.
class AndroidAssetFileSystem final : public IFileSystem {
public:
    explicit AndroidAssetFileSystem(AAssetManager* manager) noexcept : manager_(manager) {}
    AndroidAssetFileSystem(const AndroidAssetFileSystem&) = delete;
    AndroidAssetFileSystem& operator=(const AndroidAssetFileSystem&) = delete;
    [[nodiscard]] bool valid() const noexcept { return manager_ != nullptr; }
    [[nodiscard]] bool read_file(std::string_view, std::vector<std::byte>&,
        std::size_t, FileError&) const override;
    [[nodiscard]] bool write_file(std::string_view, std::span<const std::byte>,
        FileError&) override;
    [[nodiscard]] bool exists(std::string_view, bool&, FileError&) const override;
    [[nodiscard]] bool create_directories(std::string_view, FileError&) override;
    [[nodiscard]] bool remove_file(std::string_view, FileError&) override;
    // Empty path lists APK assets root. Only regular files are returned.
    [[nodiscard]] bool list_directory(std::string_view,
        std::vector<DirectoryEntry>&, FileError&) const override;
private:
    AAssetManager* manager_{nullptr};
};
} // namespace dream::platform
#endif
