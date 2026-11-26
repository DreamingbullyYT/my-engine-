#include <dream/dmod/native_library.hpp>
#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <dlfcn.h>
#endif
#include <utility>

namespace dream::dmod {
NativeLibrary::~NativeLibrary() { close(); }
NativeLibrary::NativeLibrary(NativeLibrary&& other) noexcept
    : handle_(std::exchange(other.handle_, nullptr)) {}
NativeLibrary& NativeLibrary::operator=(NativeLibrary&& other) noexcept {
    if (this != &other) { close(); handle_ = std::exchange(other.handle_, nullptr); }
    return *this;
}
bool NativeLibrary::open(const std::filesystem::path& path, std::string& error) {
    close(); error.clear();
    if (path.empty() || !path.is_absolute()) {
        error = "Native module path must be absolute."; return false;
    }
#if defined(_WIN32)
    handle_ = reinterpret_cast<void*>(LoadLibraryW(path.c_str()));
    if (!handle_) { error = "LoadLibraryW failed (Windows error " +
        std::to_string(GetLastError()) + ")."; return false; }
#elif defined(__unix__) || defined(__APPLE__)
    dlerror(); handle_ = dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);
    if (!handle_) { const auto* e = dlerror(); error = e ? e : "dlopen failed."; return false; }
#else
    (void)path; error = "Dynamic libraries are unsupported on this platform."; return false;
#endif
    return true;
}
void* NativeLibrary::symbol(const char* name, std::string& error) const noexcept {
    error.clear();
    if (!handle_ || !name || !name[0]) { error = "Library or symbol name is invalid."; return nullptr; }
#if defined(_WIN32)
    auto* p = GetProcAddress(reinterpret_cast<HMODULE>(handle_), name);
    if (!p) { error = "GetProcAddress could not find " + std::string(name); return nullptr; }
    return reinterpret_cast<void*>(p);
#elif defined(__unix__) || defined(__APPLE__)
    dlerror(); auto* p = dlsym(handle_, name);
    if (const auto* e = dlerror()) { error = e; return nullptr; }
    return p;
#else
    error = "Dynamic symbols are unsupported on this platform."; return nullptr;
#endif
}
void NativeLibrary::close() noexcept {
    if (!handle_) return;
#if defined(_WIN32)
    FreeLibrary(reinterpret_cast<HMODULE>(handle_));
#elif defined(__unix__) || defined(__APPLE__)
    dlclose(handle_);
#endif
    handle_ = nullptr;
}
} // namespace dream::dmod
