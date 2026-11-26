#include <dream/dmod/native_mod_manager.hpp>
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <memory>
#include <utility>

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
    void release() noexcept {
        if (exports && context) {
            if (enabled && exports->disable) exports->disable(context);
            enabled = false;
            if (exports->destroy) exports->destroy(context);
            context = nullptr;
        }
        exports = nullptr; library.close();
    }
    ~LoadedMod() { release(); }
};

NativeModManager::NativeModManager(NativeModHostApiV1 api) noexcept : host_api_(api) {}
NativeModManager::~NativeModManager() { shutdown(); }
bool NativeModManager::valid_host_api() const noexcept {
    return host_api_.struct_size >= sizeof(NativeModHostApiV1) &&
        host_api_.abi_version == native_mod_abi_v1;
}
NativeModResult NativeModManager::load(
    const Manifest& manifest, const std::filesystem::path& path) {
    if (shutdown_) return {NativeModError::Shutdown, "Mod manager is shut down."};
    if (!valid_host_api()) return {NativeModError::InvalidHostApi, "Host API size or ABI is invalid."};
    const auto validation = validate_manifest(manifest);
    if (!validation.valid()) return {NativeModError::InvalidManifest, validation.errors.front()};
    if (manifest.runtime_kind != ModRuntimeKind::Native || manifest.native_library.empty())
        return {NativeModError::WrongRuntime, "Manifest does not describe a native mod."};
    if (path.empty() || !path.is_absolute())
        return {NativeModError::InvalidLibraryPath, "Installed library path must be absolute."};
    if (find(manifest.id)) return {NativeModError::DuplicateMod, "Mod is already loaded: " + manifest.id};

    auto mod = std::make_unique<LoadedMod>();
    mod->manifest = manifest;
    std::string error;
    if (!mod->library.open(path, error))
        return {NativeModError::LibraryLoadFailed, std::move(error)};
    auto* symbol = mod->library.symbol(native_mod_entry_symbol_v1, error);
    if (!symbol) return {NativeModError::EntryPointMissing, std::move(error)};
    const auto entry = reinterpret_cast<NativeModEntryPointV1>(symbol);
    const auto* exports = entry();
    if (!exports || exports->struct_size < sizeof(NativeModExportsV1) ||
        exports->abi_version != native_mod_abi_v1 || !exports->create || !exports->destroy)
        return {NativeModError::InvalidPluginDescriptor, "Plugin descriptor is incomplete or ABI-incompatible."};

    const auto id = bounded_string(exports->id, 129);
    const auto name = bounded_string(exports->name, 257);
    if (id.empty() || name.empty() || id != manifest.id ||
        exports->version_major != manifest.version.major ||
        exports->version_minor != manifest.version.minor ||
        exports->version_patch != manifest.version.patch)
        return {NativeModError::IdentityMismatch, "Library identity/version does not match manifest.toml."};

    const NativeModInfoV1 info{
        static_cast<std::uint32_t>(sizeof(NativeModInfoV1)),
        mod->manifest.id.c_str(), mod->manifest.name.c_str(),
        mod->manifest.version.major, mod->manifest.version.minor, mod->manifest.version.patch
    };
    mod->exports = exports;
    mod->context = exports->create(&host_api_, &info);
    if (!mod->context) {
        mod->exports = nullptr;
        return {NativeModError::CreationFailed, "Plugin create() returned null."};
    }
    mods_.push_back(std::move(mod));
    return {};
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
    if (shutdown_) return {NativeModError::Shutdown, "Mod manager is shut down."};
    auto* mod = find(id);
    if (!mod) return {NativeModError::ModNotFound, "Native mod is not loaded."};
    if (mod->enabled) return {};
    if (mod->exports->enable && mod->exports->enable(mod->context) == 0)
        return {NativeModError::ActivationFailed, "Native mod rejected activation."};
    mod->enabled = true; return {};
}
NativeModResult NativeModManager::disable(std::string_view id) {
    if (shutdown_) return {NativeModError::Shutdown, "Mod manager is shut down."};
    auto* mod = find(id);
    if (!mod) return {NativeModError::ModNotFound, "Native mod is not loaded."};
    if (!mod->enabled) return {};
    if (mod->exports->disable) mod->exports->disable(mod->context);
    mod->enabled = false; return {};
}
NativeModResult NativeModManager::unload(std::string_view id) {
    if (shutdown_) return {NativeModError::Shutdown, "Mod manager is shut down."};
    const auto it = std::find_if(mods_.begin(), mods_.end(),
        [id](const auto& mod) { return mod->manifest.id == id; });
    if (it == mods_.end()) return {NativeModError::ModNotFound, "Native mod is not loaded."};
    (*it)->release(); mods_.erase(it); return {};
}
bool NativeModManager::update(double dt) noexcept {
    if (shutdown_ || !std::isfinite(dt) || dt < 0.0) return false;
    for (auto& mod : mods_)
        if (mod->enabled && mod->exports->update) mod->exports->update(mod->context, dt);
    return true;
}
void NativeModManager::shutdown() noexcept {
    if (shutdown_) return;
    for (auto it = mods_.rbegin(); it != mods_.rend(); ++it) (*it)->release();
    mods_.clear(); shutdown_ = true;
}
bool NativeModManager::is_loaded(std::string_view id) const noexcept { return find(id) != nullptr; }
bool NativeModManager::is_enabled(std::string_view id) const noexcept {
    const auto* mod = find(id); return mod && mod->enabled;
}
std::size_t NativeModManager::loaded_count() const noexcept { return mods_.size(); }
std::vector<NativeModStatus> NativeModManager::statuses() const {
    std::vector<NativeModStatus> result; result.reserve(mods_.size());
    for (const auto& mod : mods_) result.push_back({mod->manifest.id, mod->enabled});
    return result;
}
} // namespace dream::dmod
