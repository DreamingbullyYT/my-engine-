#pragma once

#include "core/object/object.h"
#include "core/string/string_name.h"
#include "core/variant/dictionary.h"
#include "core/variant/variant.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

struct lua_State;

struct DreamModHookInvocationV1 {
    void *next_context = nullptr;
    uint8_t (*call_next)(void *) = nullptr;
};

using DreamModNativeHookCallbackV1 =
        void (*)(void *callback_context, Dictionary *context, DreamModHookInvocationV1 *invocation) noexcept;
using DreamModNativeOriginalV1 =
        void (*)(void *original_context, Dictionary &context) noexcept;

class DreamModRuntime : public Object {
    GDCLASS(DreamModRuntime, Object);

public:
    enum HookStage : uint8_t {
        HOOK_BEFORE = 0,
        HOOK_AROUND = 1,
        HOOK_REPLACE = 2,
        HOOK_AFTER = 3,
    };

    static constexpr uint32_t NATIVE_ABI_V1 = 1;

    struct NativeHostApiV1 {
        uint32_t struct_size = 0;
        uint32_t abi_version = NATIVE_ABI_V1;
        void *runtime = nullptr;
        uint64_t (*register_hook)(
                void *, const char *, const char *, uint8_t, int32_t,
                DreamModNativeHookCallbackV1, void *) noexcept = nullptr;
        void *(*query_interface)(void *, const char *, uint32_t) noexcept = nullptr;
    };

private:
    struct ModInstance;
    struct HookEntry;
    struct DispatchState;
    struct NextContext;

    static DreamModRuntime *singleton;
    std::vector<std::unique_ptr<ModInstance>> mods;
    std::vector<HookEntry> hooks;
    std::vector<String> pending_unloads;
    uint64_t next_hook_id = 1;
    uint32_t dispatch_depth = 0;

    static int lua_register_hook(lua_State *state);
    static int lua_call_next(lua_State *state);
    static uint8_t native_call_next(void *context) noexcept;
    static void sync_dictionary_to_lua_table(lua_State *state, int table_ref, const Dictionary &dictionary);
    static void sync_lua_table_to_dictionary(lua_State *state, int table_ref, Dictionary &dictionary);
    static void push_variant(lua_State *state, const Variant &value, int depth = 0);
    static bool read_variant(lua_State *state, int index, Variant &value);
    static void push_dictionary(lua_State *state, const Dictionary &dictionary);
    static void read_dictionary(lua_State *state, int index, Dictionary &dictionary);
    static bool parse_stage(const String &value, HookStage &stage);
    static bool valid_mod_id(const String &value);
    static bool valid_package_path(const String &value);

    ModInstance *find_mod(const String &id) const;
    ModInstance *find_mod(lua_State *state) const;
    bool register_lua_hook(lua_State *state, const StringName &point, HookStage stage, int32_t priority, int function_ref);
    uint64_t register_native_hook(
            const String &owner, const StringName &point, HookStage stage, int32_t priority,
            DreamModNativeHookCallbackV1 callback, void *callback_context) noexcept;
    static uint64_t native_register_hook(
            void *runtime, const char *owner, const char *point, uint8_t stage, int32_t priority,
            DreamModNativeHookCallbackV1 callback, void *callback_context) noexcept;
    static void *native_query_interface(void *runtime, const char *name, uint32_t min_version) noexcept;

    void invoke_chain(DispatchState &dispatch, size_t index);
    void dispatch_internal(
            const StringName &point, Dictionary &context,
            DreamModNativeOriginalV1 original, void *original_context);
    void process_pending_unloads();

protected:
    static void _bind_methods();

public:
    DreamModRuntime() = default;
    ~DreamModRuntime() override;

    static DreamModRuntime *get_singleton();
    static void set_singleton(DreamModRuntime *runtime);

    bool load_mod(const String &package_path);
    bool unload_mod(const String &mod_id);
    bool enable_mod(const String &mod_id);
    bool disable_mod(const String &mod_id);
    PackedStringArray get_loaded_mods() const;
    int32_t get_hook_count() const;

    NativeHostApiV1 get_native_host_api() noexcept;
    uint64_t add_native_hook(
            const String &owner, const StringName &point, HookStage stage, int32_t priority,
            DreamModNativeHookCallbackV1 callback, void *callback_context) noexcept;

    static void dispatch_hook(
            const StringName &point, Dictionary &context,
            DreamModNativeOriginalV1 original = nullptr, void *original_context = nullptr);
};

VARIANT_ENUM_CAST(DreamModRuntime::HookStage)
