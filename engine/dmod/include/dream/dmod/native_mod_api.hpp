#pragma once
#include <cstdint>

namespace dream::dmod {
inline constexpr std::uint32_t native_mod_abi_v1 = 1;
inline constexpr char native_mod_entry_symbol_v1[] = "dream_mod_entry_v1";

enum class NativeModLogLevelV1 : std::uint32_t { Trace, Debug, Info, Warning, Error };

struct NativeModHostApiV1 final {
    std::uint32_t struct_size{};
    std::uint32_t abi_version{};
    void* host_context{};
    void* (*query_interface)(void*, const char*, std::uint32_t) noexcept{};
    void* log_context{};
    void (*log)(void*, NativeModLogLevelV1, const char*) noexcept{};
};

struct NativeModInfoV1 final {
    std::uint32_t struct_size{};
    const char* id{};
    const char* name{};
    std::uint32_t version_major{}, version_minor{}, version_patch{};
};

struct NativeModExportsV1 final {
    std::uint32_t struct_size{};
    std::uint32_t abi_version{};
    const char* id{};
    const char* name{};
    std::uint32_t version_major{}, version_minor{}, version_patch{};
    void* (*create)(const NativeModHostApiV1*, const NativeModInfoV1*) noexcept{};
    std::uint8_t (*enable)(void*) noexcept{};
    void (*update)(void*, double) noexcept{};
    void (*disable)(void*) noexcept{};
    void (*destroy)(void*) noexcept{};
};

using NativeModEntryPointV1 = const NativeModExportsV1* (*)() noexcept;
} // namespace dream::dmod

#if defined(_WIN32)
#define DREAM_DMOD_EXPORT extern "C" __declspec(dllexport)
#else
#define DREAM_DMOD_EXPORT extern "C" __attribute__((visibility("default")))
#endif
