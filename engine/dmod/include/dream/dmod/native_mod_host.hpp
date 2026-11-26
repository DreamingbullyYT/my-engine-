#pragma once
#include <dream/dmod/native_mod_api.hpp>
#include <cstdint>
#include <shared_mutex>
#include <string>
#include <string_view>
#include <unordered_map>

namespace dream::dmod {
// Exposes raw engine object addresses to trusted native mods. The engine host
// owns exposed objects and must keep them alive while mods can access them.
class NativeModHost final {
public:
    using LogSink = void (*)(void*, NativeModLogLevelV1, const char*) noexcept;
    NativeModHost() = default;
    NativeModHost(const NativeModHost&) = delete;
    NativeModHost& operator=(const NativeModHost&) = delete;

    [[nodiscard]] bool expose_interface(std::string_view id, std::uint32_t version, void* object);
    [[nodiscard]] bool remove_interface(std::string_view id);
    void set_log_sink(void* context, LogSink sink) noexcept;
    template<class T> [[nodiscard]] bool expose(std::string_view id, std::uint32_t version, T& object) {
        return expose_interface(id, version, static_cast<void*>(&object));
    }
    [[nodiscard]] NativeModHostApiV1 api() noexcept;
private:
    struct Entry { std::uint32_t version{}; void* object{}; };
    static void* query(void*, const char*, std::uint32_t) noexcept;
    static void log(void*, NativeModLogLevelV1, const char*) noexcept;
    mutable std::shared_mutex mutex_;
    std::unordered_map<std::string, Entry> interfaces_;
    void* log_context_{};
    LogSink log_sink_{};
};
} // namespace dream::dmod
