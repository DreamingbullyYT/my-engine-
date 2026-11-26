#pragma once

#include <cstddef>
#include <cstdint>
#include <shared_mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace dream::dmod {

enum class NativeHookStage : std::uint8_t {
    Before,
    Around,
    Replace,
    After
};

using NativeHookRegistrationId = std::uint64_t;

// Hook points define their own argument/result structures. user_data points to
// that caller-owned structure for the duration of dispatch. call_next is valid
// only while an Around/Replace callback is executing; each call invokes the
// remaining chain again, so the original operation may intentionally run many times.
struct NativeHookInvocationV1 final {
    void* user_data{};
    void* next_context{};
    std::uint8_t (*call_next)(void*) noexcept{};
};

using NativeHookCallbackV1 =
    void (*)(void*, NativeHookInvocationV1*) noexcept;
using NativeHookOriginalV1 = void (*)(void*) noexcept;

struct NativeHookDispatchResult final {
    bool success{false};
    bool original_called{false};
    std::size_t callbacks_invoked{0};

    [[nodiscard]] explicit operator bool() const noexcept { return success; }
};

// Native hook callbacks are unrestricted in-process code. Register callbacks
// against the owning mod ID so the manager can deactivate/remove them before
// unloading that mod's shared library.
class NativeHookRegistry final {
public:
    NativeHookRegistry() = default;
    NativeHookRegistry(const NativeHookRegistry&) = delete;
    NativeHookRegistry& operator=(const NativeHookRegistry&) = delete;

    [[nodiscard]] NativeHookRegistrationId register_hook(
        std::string_view owner_mod_id,
        std::string_view hook_point_id,
        NativeHookStage stage,
        std::int32_t priority,
        NativeHookCallbackV1 callback,
        void* callback_context) noexcept;

    [[nodiscard]] bool remove_hook(NativeHookRegistrationId registration) noexcept;
    [[nodiscard]] std::size_t registered_count() const noexcept;

private:
    friend class NativeModManager;

    struct Entry final {
        NativeHookRegistrationId registration{};
        std::string owner_mod_id;
        std::string hook_point_id;
        NativeHookStage stage{NativeHookStage::Before};
        std::int32_t priority{};
        NativeHookCallbackV1 callback{};
        void* callback_context{};
    };

    struct ChainState final {
        const std::vector<Entry>* entries{};
        NativeHookOriginalV1 original{};
        void* user_data{};
        bool original_called{false};
        std::size_t callbacks_invoked{0};
    };

    struct NextState final {
        ChainState* chain{};
        std::size_t next_index{};
    };

    [[nodiscard]] static bool valid_identifier(std::string_view value) noexcept;
    [[nodiscard]] bool set_owner_enabled(std::string_view owner, bool enabled) noexcept;
    [[nodiscard]] std::size_t remove_owner(std::string_view owner) noexcept;
    [[nodiscard]] NativeHookDispatchResult dispatch(
        std::string_view hook_point_id,
        void* user_data,
        NativeHookOriginalV1 original) noexcept;

    static std::uint8_t call_next(void* context) noexcept;
    static void invoke_chain(ChainState& state, std::size_t index) noexcept;

    mutable std::shared_mutex mutex_;
    std::vector<Entry> entries_;
    std::unordered_map<std::string, bool> owner_enabled_;
    NativeHookRegistrationId next_registration_{1};
};

} // namespace dream::dmod
