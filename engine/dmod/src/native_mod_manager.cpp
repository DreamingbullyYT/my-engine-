#include <dream/dmod/native_mod_manager.hpp>
#include <dream/dmod/resolver.hpp>
#include <dream/dmod/native_mod_package_installer.hpp>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace dream::dmod {
namespace {
std::string bounded_string(const char* value, std::size_t maximum) {
    if (!value) return {};
    std::size_t size = 0;
    while (size < maximum && value[size] != '\0') ++size;
    return size == maximum ? std::string{} : std::string(value, size);
}
}

struct NativeModManager::LoadedMod final {
    Manifest manifest;
    NativeLibrary library;
    const NativeModExportsV1* exports{};
    void* context{};
    bool enabled{};
    bool disable_requested{};
    bool unload_requested{};
};

NativeModManager::NativeModManager(NativeModHostApiV1 api) noexcept : host_api_(api) {}
NativeModManager::~NativeModManager() { shutdown(); }

bool NativeModManager::valid_host_api() const noexcept {
    std::lock_guard<std::recursive_mutex> state_lock(state_mutex_);
    return host_api_.struct_size >= sizeof(NativeModHostApiV1) &&
        host_api_.abi_version == native_mod_abi_v1;
}

void NativeModManager::begin_callback() noexcept {
    ++callback_depth_;
}

void NativeModManager::end_callback() noexcept {
    if (callback_depth_ > 0) --callback_depth_;
    if (callback_depth_ == 0) process_deferred();
}

void NativeModManager::release(LoadedMod& mod) noexcept {
    const auto owner_id = mod.manifest.id;
    if (mod.exports == nullptr || mod.context == nullptr) {
        (void)hook_registry_.remove_owner(owner_id);
        mod.exports = nullptr;
        mod.context = nullptr;
        mod.library.close();
        return;
    }

    begin_callback();
    if (mod.enabled && mod.exports->disable != nullptr) {
        mod.exports->disable(mod.context);
    }
    mod.enabled = false;
    (void)hook_registry_.set_owner_enabled(owner_id, false);
    (void)hook_registry_.remove_owner(owner_id);
    if (mod.exports->destroy != nullptr) {
        mod.exports->destroy(mod.context);
    }
    mod.context = nullptr;
    mod.exports = nullptr;
    mod.library.close();
    end_callback();
}

void NativeModManager::process_deferred() noexcept {
    if (callback_depth_ != 0 || processing_deferred_) return;
    processing_deferred_ = true;

    for (;;) {
        if (shutdown_requested_) {
            shutdown_requested_ = false;
            shutdown_ = true;
            while (!mods_.empty()) {
                auto mod = std::move(mods_.back());
                mods_.pop_back();
                release(*mod);
            }
            break;
        }

        bool processed = false;
        for (std::size_t index = mods_.size(); index > 0; --index) {
            auto& mod = *mods_[index - 1];
            if (mod.disable_requested && mod.enabled) {
                const std::string_view id = mod.manifest.id;
                mod.disable_requested = false;
                (void)disable(id);
                processed = true;
                break;
            }
        }
        if (processed) continue;

        for (std::size_t index = mods_.size(); index > 0; --index) {
            auto& mod = *mods_[index - 1];
            if (mod.unload_requested) {
                const std::string_view id = mod.manifest.id;
                (void)unload(id);
                processed = true;
                break;
            }
        }
        if (!processed) break;
    }

    processing_deferred_ = false;
}

NativeModResult NativeModManager::load(
    const Manifest& manifest, const std::filesystem::path& path) {
    std::lock_guard<std::recursive_mutex> state_lock(state_mutex_);
    if (shutdown_ || shutdown_requested_)
        return {NativeModError::Shutdown, "Mod manager is shut down."};
    if (!valid_host_api())
        return {NativeModError::InvalidHostApi, "Host API size or ABI is invalid."};
    const auto validation = validate_manifest(manifest);
    if (!validation.valid())
        return {NativeModError::InvalidManifest, validation.errors.front()};
    if (manifest.runtime_kind != ModRuntimeKind::Native || manifest.native_library.empty())
        return {NativeModError::WrongRuntime, "Manifest does not describe a native mod."};
    if (path.empty() || !path.is_absolute())
        return {NativeModError::InvalidLibraryPath, "Installed library path must be absolute."};
    if (find(manifest.id) != nullptr)
        return {NativeModError::DuplicateMod, "Mod is already loaded: " + manifest.id};

    auto mod = std::make_unique<LoadedMod>();
    mod->manifest = manifest;
    const auto loaded_id = mod->manifest.id;
    std::string error;
    if (!mod->library.open(path, error))
        return {NativeModError::LibraryLoadFailed, std::move(error)};
    auto* symbol = mod->library.symbol(native_mod_entry_symbol_v1, error);
    if (symbol == nullptr)
        return {NativeModError::EntryPointMissing, std::move(error)};
    const auto entry = reinterpret_cast<NativeModEntryPointV1>(symbol);
    const auto* exports = entry();
    if (exports == nullptr ||
        exports->struct_size < sizeof(NativeModExportsV1) ||
        exports->abi_version != native_mod_abi_v1 ||
        exports->create == nullptr || exports->destroy == nullptr) {
        return {NativeModError::InvalidPluginDescriptor,
                "Plugin descriptor is incomplete or ABI-incompatible."};
    }

    const auto id = bounded_string(exports->id, 129);
    const auto name = bounded_string(exports->name, 257);
    if (id.empty() || name.empty() || id != manifest.id ||
        exports->version_major != manifest.version.major ||
        exports->version_minor != manifest.version.minor ||
        exports->version_patch != manifest.version.patch) {
        return {NativeModError::IdentityMismatch,
                "Library identity/version does not match manifest.toml."};
    }

    const NativeModInfoV1 info{
        static_cast<std::uint32_t>(sizeof(NativeModInfoV1)),
        mod->manifest.id.c_str(), mod->manifest.name.c_str(),
        mod->manifest.version.major, mod->manifest.version.minor, mod->manifest.version.patch
    };

    // Reserve before plugin code runs: storing the module after create() must
    // not allocate or throw while the module is already executing.
    mods_.reserve(mods_.size() + 1);

    // Native modules may register hooks during create(), but those hooks
    // remain dormant until enable() succeeds.
    if (!hook_registry_.set_owner_enabled(loaded_id, false))
        return {NativeModError::CreationFailed, "Cannot track native hook ownership."};
    mod->exports = exports;
    begin_callback();
    mod->context = exports->create(&host_api_, &info);
    if (mod->context == nullptr) {
        (void)hook_registry_.remove_owner(loaded_id);
        mod->exports = nullptr;
        end_callback();
        return {NativeModError::CreationFailed, "Plugin create() returned null."};
    }
    mods_.push_back(std::move(mod));
    end_callback();
    if (shutdown_ || !is_loaded(loaded_id))
        return {NativeModError::Shutdown, "Mod manager shut down during native module creation."};
    return {};
}

NativeModResult NativeModManager::install_and_load(
    const PackageReader& package, const std::filesystem::path& private_mod_root) {
    std::lock_guard<std::recursive_mutex> state_lock(state_mutex_);
    if (shutdown_ || shutdown_requested_)
        return {NativeModError::Shutdown, "Mod manager is shut down."};
    const auto installed = NativeModPackageInstaller::install(package, private_mod_root);
    if (!installed.success())
        return {NativeModError::PackageInstallFailed, installed.error};
    return load(package.manifest(), installed.library_path);
}

NativeModBatchResult NativeModManager::install_and_load_all(
    std::span<const PackageReader* const> packages,
    const std::filesystem::path& private_mod_root,
    core::Version engine_api_version) {
    std::lock_guard<std::recursive_mutex> state_lock(state_mutex_);
    NativeModBatchResult result;
    std::vector<NativeModPackage> installed_packages;
    installed_packages.reserve(packages.size());
    for (const auto* package : packages) {
        if (package == nullptr) {
            result.error = NativeModError::InvalidManifest;
            result.message = "Native package list contains a null reader.";
            return result;
        }
        if (package->manifest().runtime_kind != ModRuntimeKind::Native) {
            result.error = NativeModError::WrongRuntime;
            result.message = "Every package in a native install batch must use the native runtime.";
            return result;
        }
        const auto installed = NativeModPackageInstaller::install(*package, private_mod_root);
        if (!installed.success()) {
            result.error = NativeModError::PackageInstallFailed;
            result.message = package->manifest().id + ": " + installed.error;
            return result;
        }
        installed_packages.push_back({package->manifest(), installed.library_path});
    }
    return load_all(installed_packages, engine_api_version);
}

NativeModBatchResult NativeModManager::load_all(
    std::span<const NativeModPackage> packages, core::Version engine_api_version) {
    std::lock_guard<std::recursive_mutex> state_lock(state_mutex_);
    NativeModBatchResult result;
    if (shutdown_ || shutdown_requested_) {
        result.error = NativeModError::Shutdown;
        result.message = "Mod manager is shut down.";
        return result;
    }
    if (!valid_host_api()) {
        result.error = NativeModError::InvalidHostApi;
        result.message = "Host API size or ABI is invalid.";
        return result;
    }

    std::vector<Manifest> manifests;
    manifests.reserve(packages.size());
    for (const auto& package : packages) {
        if (package.manifest.runtime_kind != ModRuntimeKind::Native) {
            result.error = NativeModError::WrongRuntime;
            result.message = "Every package in a native load batch must use the native runtime.";
            return result;
        }
        if (package.installed_library_path.empty() ||
            !package.installed_library_path.is_absolute()) {
            result.error = NativeModError::InvalidLibraryPath;
            result.message = "Every installed native library path must be absolute.";
            return result;
        }
        if (find(package.manifest.id) != nullptr) {
            result.error = NativeModError::DuplicateMod;
            result.message = "Mod is already loaded: " + package.manifest.id;
            return result;
        }
        manifests.push_back(package.manifest);
    }

    const auto resolution = resolve_load_order(manifests, engine_api_version);
    if (!resolution.success()) {
        result.error = NativeModError::InvalidManifest;
        result.message = resolution.errors.empty()
            ? "Native package dependency resolution failed."
            : resolution.errors.front();
        return result;
    }

    result.load_order = resolution.load_order;
    std::vector<std::string> loaded;
    loaded.reserve(packages.size());
    begin_callback();
    for (const auto& id : resolution.load_order) {
        if (shutdown_requested_) {
            result.error = NativeModError::Shutdown;
            result.message = "Mod manager shut down during batch loading.";
            break;
        }
        const auto package = std::find_if(packages.begin(), packages.end(),
            [&id](const NativeModPackage& item) { return item.manifest.id == id; });
        if (package == packages.end()) {
            result.error = NativeModError::InvalidManifest;
            result.message = "Resolved package is missing from the load batch: " + id;
            break;
        }
        const auto loaded_result = load(package->manifest, package->installed_library_path);
        if (!loaded_result) {
            result.error = loaded_result.error;
            result.message = id + ": " + loaded_result.message;
            break;
        }
        loaded.push_back(id);
    }

    if (result.error != NativeModError::None) {
        for (auto it = loaded.rbegin(); it != loaded.rend(); ++it) {
            (void)unload(*it);
        }
        result.load_order.clear();
    }
    end_callback();
    return result;
}

NativeModBatchResult NativeModManager::enable_all() {
    std::lock_guard<std::recursive_mutex> state_lock(state_mutex_);
    NativeModBatchResult result;
    if (shutdown_ || shutdown_requested_) {
        result.error = NativeModError::Shutdown;
        result.message = "Mod manager is shut down.";
        return result;
    }

    std::vector<std::string> ids;
    ids.reserve(mods_.size());
    for (const auto& mod : mods_) ids.push_back(mod->manifest.id);

    std::vector<std::string> enabled;
    enabled.reserve(ids.size());
    begin_callback();
    for (const auto& id : ids) {
        const auto enabled_result = enable(id);
        if (!enabled_result) {
            result.error = enabled_result.error;
            result.message = id + ": " + enabled_result.message;
            for (auto it = enabled.rbegin(); it != enabled.rend(); ++it) {
                (void)disable(*it);
            }
            break;
        }
        enabled.push_back(id);
        result.load_order.push_back(id);
    }
    if (result.error != NativeModError::None) result.load_order.clear();
    end_callback();
    return result;
}

NativeModManager::LoadedMod* NativeModManager::find(std::string_view id) noexcept {
    const auto it = std::find_if(mods_.begin(), mods_.end(),
        [id](const auto& mod) { return mod->manifest.id == id; });
    return it == mods_.end() ? nullptr : it->get();
}

const NativeModManager::LoadedMod* NativeModManager::find(std::string_view id) const noexcept {
    const auto it = std::find_if(mods_.begin(), mods_.end(),
        [id](const auto& mod) { return mod->manifest.id == id; });
    return it == mods_.end() ? nullptr : it->get();
}

NativeModResult NativeModManager::enable(std::string_view id) {
    std::lock_guard<std::recursive_mutex> state_lock(state_mutex_);
    if (shutdown_ || shutdown_requested_)
        return {NativeModError::Shutdown, "Mod manager is shut down."};
    auto* mod = find(id);
    if (mod == nullptr || mod->unload_requested)
        return {NativeModError::ModNotFound, "Native mod is not loaded."};
    if (mod->enabled) return {};
    if (!hook_registry_.set_owner_enabled(id, true))
        return {NativeModError::ActivationFailed, "Cannot activate native hook registrations."};
    if (mod->exports->enable != nullptr) {
        begin_callback();
        const auto accepted = mod->exports->enable(mod->context) != 0;
        if (accepted) {
            mod->enabled = true;
        } else {
            (void)hook_registry_.set_owner_enabled(id, false);
            (void)hook_registry_.remove_owner(id);
        }
        end_callback();
        if (!accepted)
            return {NativeModError::ActivationFailed, "Native mod rejected activation."};
        return {};
    }
    mod->enabled = true;
    return {};
}

NativeModResult NativeModManager::disable(std::string_view id) {
    std::lock_guard<std::recursive_mutex> state_lock(state_mutex_);
    if (shutdown_ || shutdown_requested_)
        return {NativeModError::Shutdown, "Mod manager is shut down."};
    auto* mod = find(id);
    if (mod == nullptr || mod->unload_requested)
        return {NativeModError::ModNotFound, "Native mod is not loaded."};
    if (!mod->enabled) return {};
    if (callback_depth_ != 0) {
        mod->disable_requested = true;
        (void)hook_registry_.set_owner_enabled(id, false);
        return {};
    }
    mod->disable_requested = false;
    if (mod->exports->disable != nullptr) {
        begin_callback();
        mod->exports->disable(mod->context);
        mod->enabled = false;
        (void)hook_registry_.set_owner_enabled(id, false);
        end_callback();
    } else {
        mod->enabled = false;
        (void)hook_registry_.set_owner_enabled(id, false);
    }
    return {};
}

NativeModResult NativeModManager::unload(std::string_view id) {
    std::lock_guard<std::recursive_mutex> state_lock(state_mutex_);
    if (shutdown_ || shutdown_requested_)
        return {NativeModError::Shutdown, "Mod manager is shut down."};
    const auto it = std::find_if(mods_.begin(), mods_.end(),
        [id](const auto& mod) { return mod->manifest.id == id; });
    if (it == mods_.end())
        return {NativeModError::ModNotFound, "Native mod is not loaded."};
    if (callback_depth_ != 0) {
        (*it)->unload_requested = true;
        (*it)->disable_requested = false;
        (void)hook_registry_.set_owner_enabled(id, false);
        return {};
    }

    auto victim = std::move(*it);
    mods_.erase(it);
    release(*victim);
    return {};
}

NativeHookRegistry& NativeModManager::hook_registry() noexcept {
    return hook_registry_;
}

NativeHookDispatchResult NativeModManager::dispatch_hooks(
    std::string_view hook_point_id,
    void* user_data,
    NativeHookOriginalV1 original) noexcept {
    std::lock_guard<std::recursive_mutex> state_lock(state_mutex_);
    if (original == nullptr) return {};
    if (shutdown_ || shutdown_requested_) {
        original(user_data);
        return {true, true, 0};
    }

    begin_callback();
    auto result = hook_registry_.dispatch(hook_point_id, user_data, original);
    if (!result.success) {
        // A broken/invalid hook registration must never suppress the game operation.
        original(user_data);
        result = {true, true, 0};
    }
    end_callback();
    return result;
}

bool NativeModManager::update(double delta_seconds) noexcept {
    std::lock_guard<std::recursive_mutex> state_lock(state_mutex_);
    if (shutdown_ || shutdown_requested_ ||
        !std::isfinite(delta_seconds) || delta_seconds < 0.0) return false;
    begin_callback();
    for (const auto& mod : mods_) {
        if (shutdown_requested_) break;
        if (mod->enabled && !mod->disable_requested && !mod->unload_requested &&
            mod->exports != nullptr && mod->exports->update != nullptr) {
            mod->exports->update(mod->context, delta_seconds);
        }
    }
    end_callback();
    return true;
}

void NativeModManager::shutdown() noexcept {
    std::lock_guard<std::recursive_mutex> state_lock(state_mutex_);
    if (shutdown_ || shutdown_requested_) return;
    shutdown_requested_ = true;
    process_deferred();
}

bool NativeModManager::is_loaded(std::string_view id) const noexcept {
    std::lock_guard<std::recursive_mutex> state_lock(state_mutex_);
    return find(id) != nullptr;
}

bool NativeModManager::is_enabled(std::string_view id) const noexcept {
    std::lock_guard<std::recursive_mutex> state_lock(state_mutex_);
    const auto* mod = find(id);
    return mod != nullptr && mod->enabled && !mod->unload_requested;
}

std::size_t NativeModManager::loaded_count() const noexcept {
    std::lock_guard<std::recursive_mutex> state_lock(state_mutex_);
    return mods_.size();
}

std::vector<NativeModStatus> NativeModManager::statuses() const {
    std::lock_guard<std::recursive_mutex> state_lock(state_mutex_);
    std::vector<NativeModStatus> result;
    result.reserve(mods_.size());
    for (const auto& mod : mods_)
        result.push_back({mod->manifest.id, mod->enabled && !mod->unload_requested});
    return result;
}

} // namespace dream::dmod
