#include <dream/dmod/native_mod_host.hpp>
#include <mutex>
namespace dream::dmod {
namespace {
bool valid_id(std::string_view id) noexcept {
    if (id.empty() || id.size() > 128) return false;
    for (unsigned char c : id)
        if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') ||
              c == '.' || c == '_' || c == '-')) return false;
    return true;
}
}
bool NativeModHost::expose_interface(std::string_view id, std::uint32_t version, void* object) {
    if (!valid_id(id) || version == 0 || !object) return false;
    std::unique_lock lock(mutex_);
    return interfaces_.emplace(std::string(id), Entry{version, object}).second;
}
bool NativeModHost::remove_interface(std::string_view id) {
    if (!valid_id(id)) return false;
    std::unique_lock lock(mutex_);
    return interfaces_.erase(std::string(id)) != 0;
}
void NativeModHost::set_log_sink(void* context, LogSink sink) noexcept {
    std::unique_lock lock(mutex_); log_context_ = context; log_sink_ = sink;
}
NativeModHostApiV1 NativeModHost::api() noexcept {
    return {static_cast<std::uint32_t>(sizeof(NativeModHostApiV1)), native_mod_abi_v1,
            this, &NativeModHost::query, this, &NativeModHost::log};
}
void* NativeModHost::query(void* context, const char* id, std::uint32_t minimum) noexcept {
    if (!context || !id || minimum == 0) return nullptr;
    auto& host = *static_cast<NativeModHost*>(context);
    try {
        std::shared_lock lock(host.mutex_);
        const auto it = host.interfaces_.find(id);
        return it != host.interfaces_.end() && it->second.version >= minimum
            ? it->second.object : nullptr;
    } catch (...) { return nullptr; }
}
void NativeModHost::log(void* context, NativeModLogLevelV1 level, const char* message) noexcept {
    if (!context || !message) return;
    auto& host = *static_cast<NativeModHost*>(context);
    LogSink sink{}; void* sink_context{};
    try {
        std::shared_lock lock(host.mutex_);
        sink = host.log_sink_; sink_context = host.log_context_;
    } catch (...) { return; }
    if (sink) sink(sink_context, level, message);
}
} // namespace dream::dmod
