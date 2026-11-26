#pragma once
#include <dream/dmod/manifest.hpp>
#include <dream/dmod/native_library.hpp>
#include <dream/dmod/native_mod_api.hpp>
#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace dream::dmod {
enum class NativeModError {
    None, InvalidHostApi, InvalidManifest, WrongRuntime, InvalidLibraryPath,
    DuplicateMod, LibraryLoadFailed, EntryPointMissing, InvalidPluginDescriptor,
    IdentityMismatch, CreationFailed, ModNotFound, ActivationFailed, Shutdown
};
struct NativeModResult final {
    NativeModError error{NativeModError::None};
    std::string message;
    [[nodiscard]] explicit operator bool() const noexcept { return error == NativeModError::None; }
};
struct NativeModStatus final { std::string id; bool enabled{false}; };

// Native modules execute in-process without an engine sandbox. A module can
// change engine/game state, memory and application data. Load only trusted mods.
class NativeModManager final {
public:
    explicit NativeModManager(NativeModHostApiV1 host_api) noexcept;
    ~NativeModManager();
    NativeModManager(const NativeModManager&) = delete;
    NativeModManager& operator=(const NativeModManager&) = delete;

    [[nodiscard]] NativeModResult load(const Manifest&, const std::filesystem::path& installed_library_path);
    [[nodiscard]] NativeModResult enable(std::string_view id);
    [[nodiscard]] NativeModResult disable(std::string_view id);
    [[nodiscard]] NativeModResult unload(std::string_view id);
    [[nodiscard]] bool update(double delta_seconds) noexcept;
    // Unloads in reverse load order. Mods must stop their own threads and
    // unregister callbacks/hooks before disable/destroy returns.
    void shutdown() noexcept;
    [[nodiscard]] bool valid_host_api() const noexcept;
    [[nodiscard]] bool is_loaded(std::string_view id) const noexcept;
    [[nodiscard]] bool is_enabled(std::string_view id) const noexcept;
    [[nodiscard]] std::size_t loaded_count() const noexcept;
    [[nodiscard]] std::vector<NativeModStatus> statuses() const;
private:
    struct LoadedMod;
    [[nodiscard]] LoadedMod* find(std::string_view) noexcept;
    [[nodiscard]] const LoadedMod* find(std::string_view) const noexcept;
    NativeModHostApiV1 host_api_{};
    std::vector<std::unique_ptr<LoadedMod>> mods_;
    bool shutdown_{false};
};
} // namespace dream::dmod
