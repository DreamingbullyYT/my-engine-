#pragma once
#include <filesystem>
#include <string>

namespace dream::dmod {
class NativeLibrary final {
public:
    NativeLibrary() = default;
    ~NativeLibrary();
    NativeLibrary(const NativeLibrary&) = delete;
    NativeLibrary& operator=(const NativeLibrary&) = delete;
    NativeLibrary(NativeLibrary&&) noexcept;
    NativeLibrary& operator=(NativeLibrary&&) noexcept;

    [[nodiscard]] bool open(const std::filesystem::path& absolute_path, std::string& error);
    [[nodiscard]] void* symbol(const char* name, std::string& error) const noexcept;
    void close() noexcept;
    [[nodiscard]] bool loaded() const noexcept { return handle_ != nullptr; }
private:
    void* handle_{};
};
} // namespace dream::dmod
