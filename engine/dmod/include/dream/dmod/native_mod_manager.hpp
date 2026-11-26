#pragma once
#include <dream/dmod/manifest.hpp>
#include <dream/dmod/native_library.hpp>
#include <dream/dmod/package_reader.hpp>
#include <dream/core/version.hpp>
#include <dream/dmod/native_mod_api.hpp>
#include <dream/dmod/native_hook_registry.hpp>
#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <span>
#include <vector>

namespace dream::dmod {
enum class NativeModError {
    None, InvalidHostApi, InvalidManifest, WrongRuntime, InvalidLibraryPath,
    DuplicateMod, LibraryLoadFailed, EntryPointMissing, InvalidPluginDescriptor,
    IdentityMismatch, CreationFailed, ModNotFound, ActivationFailed, PackageInstallFailed, Shutdown
};
struct NativeModResult final {
    NativeModError error{NativeModError::None};
    std::string message;
    [[nodiscard]] explicit operator bool() const noexcept { return error == NativeModError::None; }
};
struct NativeModStatus final { std::string id; bool enabled{false}; };

struct NativeModPackage final {
    Manifest manifest;
    std::filesystem::path installed_library_path;
};

struct NativeModBatchResult final {
    NativeModError error{NativeModError::None};
    std::string message;
    std::vector<std::string> load_order;
    [[nodiscard]] explicit operator bool() const noexcept {
        return error == NativeModError::None;
    }
};

// Native modules execute in-process without an engine sandbox. A module can
// change engine/game state, memory and application data. Load only trusted mods.
class NativeModManager final {
public:
    explicit NativeModManager(NativeModHostApiV1 host_api) noexcept;
    ~NativeModManager();
    NativeModManager(const NativeModManager&) = delete;
    NativeModManager& operator=(const NativeModManager&) = delete;

    [[nodiscard]] NativeModResult load(const Manifest&, const std::filesystem::path& installed_library_path);
    // Installs the declared library from a validated .dmod archive, then loads it.
    [[nodiscard]] NativeModResult install_and_load(
        const PackageReader& package, const std::filesystem::path& private_mod_root);
    // Installs all packages first, then resolves and loads the native dependency graph.
    [[nodiscard]] NativeModBatchResult install_and_load_all(
        std::span<const PackageReader* const> packages,
        const std::filesystem::path& private_mod_root,
        core::Version engine_api_version);
    // Resolves dependencies, loads in deterministic dependency-first order,
    // and unloads every module added by this call if any load fails.
    [[nodiscard]] NativeModBatchResult load_all(
        std::span<const NativeModPackage> packages, core::Version engine_api_version);
    // Enables in load order; disables modules enabled by this call on failure.
    [[nodiscard]] NativeModBatchResult enable_all();
    [[nodiscard]] NativeModResult enable(std::string_view id);
    [[nodiscard]] NativeModResult disable(std::string_view id);
    [[nodiscard]] NativeModResult unload(std::string_view id);
    [[nodiscard]] bool update(double delta_seconds) noexcept;
    // Unloads in reverse load order. Mods must stop their own threads and
    // unregister callbacks/hooks before disable/destroy returns.
    void shutdown() noexcept;
    [[nodiscard]] NativeHookRegistry& hook_registry() noexcept;
    [[nodiscard]] NativeHookDispatchResult dispatch_hooks(
        std::string_view hook_point_id, void* user_data,
        NativeHookOriginalV1 original) noexcept;
    [[nodiscard]] bool valid_host_api() const noexcept;
    [[nodiscard]] bool is_loaded(std::string_view id) const noexcept;
    [[nodiscard]] bool is_enabled(std::string_view id) const noexcept;
    [[nodiscard]] std::size_t loaded_count() const noexcept;
    [[nodiscard]] std::vector<NativeModStatus> statuses() const;
private:
    struct LoadedMod;
    void begin_callback() noexcept;
    void end_callback() noexcept;
    void process_deferred() noexcept;
    void release(LoadedMod& mod) noexcept;
    [[nodiscard]] LoadedMod* find(std::string_view) noexcept;
    [[nodiscard]] const LoadedMod* find(std::string_view) const noexcept;
    NativeModHostApiV1 host_api_{};
    NativeHookRegistry hook_registry_{};
    std::vector<std::unique_ptr<LoadedMod>> mods_;
    std::size_t callback_depth_{0};
    bool processing_deferred_{false};
    bool shutdown_requested_{false};
    bool shutdown_{false};
};
} // namespace dream::dmod
