#include "dream_mod_runtime.h"

#include "core/config/engine.h"
#include "core/input/input.h"
#include "core/io/file_access.h"
#include "core/io/dir_access.h"
#include "core/config/project_settings.h"
#include "core/io/json.h"
#include "core/object/class_db.h"
#include "core/object/object_id.h"
#include "core/os/os.h"
#include "core/string/print_string.h"
#include "modules/zip/zip_reader.h"
#include "scene/main/scene_tree.h"
#include "servers/rendering/rendering_server.h"

#ifndef PHYSICS_2D_DISABLED
#include "servers/physics_2d/physics_server_2d.h"
#endif
#ifndef PHYSICS_3D_DISABLED
#include "servers/physics_3d/physics_server_3d.h"
#endif

#include <algorithm>
#include <cstring>
#include <limits>
#include <cmath>
#include <utility>

extern "C" {
#include <lauxlib.h>
#include <lualib.h>
}

struct DreamModRuntime::ModInstance {
    String id;
    String name;
    String package_path;
    lua_State *state = nullptr;
    bool enabled = false;
    void *native_library_handle = nullptr;
    void *native_context = nullptr;
    const NativeModExportsV1 *native_exports = nullptr;
    NativeHostApiV1 native_api;

    ~ModInstance() {
        if (state != nullptr) {
            lua_close(state);
            state = nullptr;
        }
    }
};

struct DreamModRuntime::HookEntry {
    uint64_t registration = 0;
    String owner;
    StringName point;
    HookStage stage = HOOK_BEFORE;
    int32_t priority = 0;
    uint64_t order = 0;
    lua_State *lua = nullptr;
    int lua_ref = LUA_NOREF;
    DreamModNativeHookCallbackV1 native_callback = nullptr;
    void *native_context = nullptr;
};

struct DreamModRuntime::DispatchState {
    DreamModRuntime *runtime = nullptr;
    Dictionary *context = nullptr;
    DreamModNativeOriginalV1 original = nullptr;
    void *original_context = nullptr;
    std::vector<HookEntry> chain;
    bool original_called = false;
};

struct DreamModRuntime::NextContext {
    DispatchState *dispatch = nullptr;
    size_t next_index = 0;
    lua_State *lua = nullptr;
    int table_ref = LUA_NOREF;
    uint32_t call_count = 0;
};

uint8_t DreamModRuntime::native_call_next(void *p_context) noexcept {
    if (p_context == nullptr) return 0;
    auto *next = static_cast<NextContext *>(p_context);
    if (next->dispatch == nullptr || next->dispatch->runtime == nullptr) return 0;
    ++next->call_count;
    if (next->lua != nullptr && next->table_ref != LUA_NOREF) {
        sync_lua_table_to_dictionary(next->lua, next->table_ref, *next->dispatch->context);
    }
    next->dispatch->runtime->invoke_chain(*next->dispatch, next->next_index);
    if (next->lua != nullptr && next->table_ref != LUA_NOREF) {
        sync_dictionary_to_lua_table(next->lua, next->table_ref, *next->dispatch->context);
    }
    return 1;
}

DreamModRuntime *DreamModRuntime::singleton = nullptr;

DreamModRuntime *DreamModRuntime::get_singleton() {
    return singleton;
}

void DreamModRuntime::set_singleton(DreamModRuntime *runtime) {
    singleton = runtime;
}

DreamModRuntime::~DreamModRuntime() {
    while (!mods.empty()) {
        const String id = mods.back()->id;
        if (!unload_mod(id)) break;
    }
    hooks.clear();
    if (singleton == this) {
        singleton = nullptr;
    }
}

bool DreamModRuntime::valid_mod_id(const String &value) {
    if (value.is_empty() || value.length() > 128) {
        return false;
    }
    bool segment_start = true;
    for (int i = 0; i < value.length(); ++i) {
        const char32_t c = value[i];
        if (c == '.') {
            if (segment_start) {
                return false;
            }
            segment_start = true;
            continue;
        }
        if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_' || c == '-')) {
            return false;
        }
        segment_start = false;
    }
    return !segment_start;
}

bool DreamModRuntime::valid_package_path(const String &value) {
    if (value.is_empty() || value.begins_with("/") || value.contains("\\") || value.contains(":")) {
        return false;
    }
    PackedStringArray parts = value.split("/");
    for (int i = 0; i < parts.size(); ++i) {
        if (parts[i].is_empty() || parts[i] == "." || parts[i] == "..") {
            return false;
        }
    }
    return true;
}

bool DreamModRuntime::parse_stage(const String &value, HookStage &stage) {
    if (value == "Before") {
        stage = HOOK_BEFORE;
    } else if (value == "Around") {
        stage = HOOK_AROUND;
    } else if (value == "Replace") {
        stage = HOOK_REPLACE;
    } else if (value == "After") {
        stage = HOOK_AFTER;
    } else {
        return false;
    }
    return true;
}

DreamModRuntime::ModInstance *DreamModRuntime::find_mod(const String &id) const {
    for (const std::unique_ptr<ModInstance> &mod : mods) {
        if (mod->id == id) {
            return mod.get();
        }
    }
    return nullptr;
}

DreamModRuntime::ModInstance *DreamModRuntime::find_mod(lua_State *state) const {
    for (const std::unique_ptr<ModInstance> &mod : mods) {
        if (mod->state == state) {
            return mod.get();
        }
    }
    return nullptr;
}

void DreamModRuntime::push_variant(lua_State *state, const Variant &value, int depth) {
    if (depth > 4) {
        lua_pushnil(state);
        return;
    }
    switch (value.get_type()) {
        case Variant::NIL:
            lua_pushnil(state);
            break;
        case Variant::BOOL:
            lua_pushboolean(state, static_cast<bool>(value));
            break;
        case Variant::INT:
            lua_pushinteger(state, static_cast<lua_Integer>(static_cast<int64_t>(value)));
            break;
        case Variant::FLOAT:
            lua_pushnumber(state, static_cast<lua_Number>(static_cast<double>(value)));
            break;
        case Variant::STRING: {
            CharString utf8 = String(value).utf8();
            lua_pushlstring(state, utf8.get_data(), utf8.length());
        } break;
        case Variant::DICTIONARY: {
            Dictionary dictionary = value;
            push_dictionary(state, dictionary);
        } break;
        case Variant::ARRAY: {
            Array array = value;
            lua_createtable(state, array.size(), 0);
            for (int i = 0; i < array.size(); ++i) {
                push_variant(state, array[i], depth + 1);
                lua_rawseti(state, -2, i + 1);
            }
        } break;
        case Variant::OBJECT: {
            Object *object = value;
            lua_pushinteger(state, object != nullptr ? static_cast<lua_Integer>(object->get_instance_id()) : 0);
        } break;
        default:
            lua_pushnil(state);
            break;
    }
}

bool DreamModRuntime::read_variant(lua_State *state, int index, Variant &value) {
    switch (lua_type(state, index)) {
        case LUA_TNIL:
            value = Variant();
            return true;
        case LUA_TBOOLEAN:
            value = static_cast<bool>(lua_toboolean(state, index));
            return true;
        case LUA_TNUMBER:
            if (lua_isinteger(state, index)) {
                value = static_cast<int64_t>(lua_tointeger(state, index));
            } else {
                value = static_cast<double>(lua_tonumber(state, index));
            }
            return true;
        case LUA_TSTRING: {
            size_t length = 0;
            const char *text = lua_tolstring(state, index, &length);
            value = String::utf8(text, static_cast<int64_t>(length));
            return true;
        }
        default:
            return false;
    }
}

void DreamModRuntime::sync_dictionary_to_lua_table(
        lua_State *state, int table_ref, const Dictionary &dictionary) {
    lua_rawgeti(state, LUA_REGISTRYINDEX, table_ref);
    const int table_index = lua_absindex(state, -1);
    lua_pushnil(state);
    while (lua_next(state, table_index) != 0) {
        lua_pop(state, 1);
        lua_pushvalue(state, -1);
        lua_pushnil(state);
        lua_settable(state, table_index);
    }
    Array keys = dictionary.keys();
    for (int i = 0; i < keys.size(); ++i) {
        Variant key = keys[i];
        if (key.get_type() != Variant::STRING) continue;
        CharString utf8 = String(key).utf8();
        lua_pushlstring(state, utf8.get_data(), utf8.length());
        push_variant(state, dictionary[key]);
        lua_settable(state, table_index);
    }
    lua_pop(state, 1);
}

void DreamModRuntime::sync_lua_table_to_dictionary(
        lua_State *state, int table_ref, Dictionary &dictionary) {
    lua_rawgeti(state, LUA_REGISTRYINDEX, table_ref);
    read_dictionary(state, -1, dictionary);
    lua_pop(state, 1);
}

void DreamModRuntime::push_dictionary(lua_State *state, const Dictionary &dictionary) {
    lua_createtable(state, 0, dictionary.size());
    Array keys = dictionary.keys();
    for (int i = 0; i < keys.size(); ++i) {
        Variant key = keys[i];
        if (key.get_type() != Variant::STRING) {
            continue;
        }
        CharString utf8 = String(key).utf8();
        lua_pushlstring(state, utf8.get_data(), utf8.length());
        push_variant(state, dictionary[key]);
        lua_settable(state, -3);
    }
}

void DreamModRuntime::read_dictionary(lua_State *state, int index, Dictionary &dictionary) {
    const int absolute_index = lua_absindex(state, index);
    dictionary.clear();
    lua_pushnil(state);
    while (lua_next(state, absolute_index) != 0) {
        if (lua_type(state, -2) == LUA_TSTRING) {
            size_t key_length = 0;
            const char *key_data = lua_tolstring(state, -2, &key_length);
            Variant value;
            if (read_variant(state, -1, value)) {
                dictionary[String::utf8(key_data, static_cast<int64_t>(key_length))] = value;
            }
        }
        lua_pop(state, 1);
    }
}

bool DreamModRuntime::register_lua_hook(
        lua_State *state, const StringName &point, HookStage stage, int32_t priority, int function_ref) {
    ModInstance *owner = find_mod(state);
    if (owner == nullptr || !valid_mod_id(owner->id) || point.is_empty() || function_ref == LUA_NOREF) {
        return false;
    }
    if (next_hook_id == 0 || next_hook_id == std::numeric_limits<uint64_t>::max()) {
        return false;
    }
    HookEntry entry;
    entry.registration = next_hook_id++;
    entry.owner = owner->id;
    entry.point = point;
    entry.stage = stage;
    entry.priority = priority;
    entry.order = entry.registration;
    entry.lua = state;
    entry.lua_ref = function_ref;
    hooks.push_back(entry);
    return true;
}

uint64_t DreamModRuntime::register_native_hook(
        const String &owner, const StringName &point, HookStage stage, int32_t priority,
        DreamModNativeHookCallbackV1 callback, void *callback_context) noexcept {
    if (!valid_mod_id(owner) || point.is_empty() || callback == nullptr ||
            stage < HOOK_BEFORE || stage > HOOK_AFTER ||
            next_hook_id == 0 || next_hook_id == std::numeric_limits<uint64_t>::max()) {
        return 0;
    }
    HookEntry entry;
    entry.registration = next_hook_id++;
    entry.owner = owner;
    entry.point = point;
    entry.stage = stage;
    entry.priority = priority;
    entry.order = entry.registration;
    entry.native_callback = callback;
    entry.native_context = callback_context;
    hooks.push_back(entry);
    return entry.registration;
}

uint64_t DreamModRuntime::native_register_hook(
        void *runtime, const char *owner, const char *point, uint8_t stage, int32_t priority,
        DreamModNativeHookCallbackV1 callback, void *callback_context) noexcept {
    if (runtime == nullptr || owner == nullptr || point == nullptr || stage > HOOK_AFTER) return 0;
    return static_cast<DreamModRuntime *>(runtime)->register_native_hook(
            String::utf8(owner), StringName(String::utf8(point)),
            static_cast<HookStage>(stage), priority, callback, callback_context);
}

void *DreamModRuntime::native_query_interface(void *runtime, const char *name, uint32_t min_version) noexcept {
    if (runtime == nullptr || name == nullptr || min_version > 1) {
        return nullptr;
    }
    if (std::strcmp(name, "godot.engine") == 0) {
        return Engine::get_singleton();
    }
    if (std::strcmp(name, "godot.scene_tree") == 0) {
        return SceneTree::get_singleton();
    }
    if (std::strcmp(name, "godot.input") == 0) {
        return Input::get_singleton();
    }
    if (std::strcmp(name, "godot.rendering_server") == 0) {
        return RenderingServer::get_singleton();
    }
    if (std::strcmp(name, "godot.os") == 0) {
        return OS::get_singleton();
    }
#ifndef PHYSICS_2D_DISABLED
    if (std::strcmp(name, "godot.physics_server_2d") == 0) {
        return PhysicsServer2D::get_singleton();
    }
#endif
#ifndef PHYSICS_3D_DISABLED
    if (std::strcmp(name, "godot.physics_server_3d") == 0) {
        return PhysicsServer3D::get_singleton();
    }
#endif
    return nullptr;
}

uint64_t DreamModRuntime::add_native_hook(
        const String &owner, const StringName &point, HookStage stage, int32_t priority,
        DreamModNativeHookCallbackV1 callback, void *callback_context) noexcept {
    return register_native_hook(owner, point, stage, priority, callback, callback_context);
}

DreamModRuntime::NativeHostApiV1 DreamModRuntime::get_native_host_api() noexcept {
    NativeHostApiV1 api;
    api.struct_size = sizeof(NativeHostApiV1);
    api.abi_version = NATIVE_ABI_V1;
    api.runtime = this;
    api.register_hook = &DreamModRuntime::native_register_hook;
    api.query_interface = &DreamModRuntime::native_query_interface;
    return api;
}

int DreamModRuntime::lua_register_hook(lua_State *state) {
    DreamModRuntime *runtime = get_singleton();
    if (runtime == nullptr || lua_gettop(state) < 3 || lua_type(state, 3) != LUA_TFUNCTION) {
        lua_pushnil(state);
        return 1;
    }

    const char *point_text = luaL_checkstring(state, 1);
    const char *stage_text = luaL_checkstring(state, 2);
    const int32_t priority = static_cast<int32_t>(luaL_optinteger(state, 4, 0));
    HookStage stage;
    if (point_text == nullptr || stage_text == nullptr ||
            !parse_stage(String::utf8(stage_text), stage)) {
        lua_pushnil(state);
        return 1;
    }

    StringName point(String::utf8(point_text));
    lua_pushvalue(state, 3);
    const int function_ref = luaL_ref(state, LUA_REGISTRYINDEX);
    if (!runtime->register_lua_hook(state, point, stage, priority, function_ref)) {
        luaL_unref(state, LUA_REGISTRYINDEX, function_ref);
        lua_pushnil(state);
        return 1;
    }
    lua_pushinteger(state, static_cast<lua_Integer>(runtime->next_hook_id - 1));
    return 1;
}

int DreamModRuntime::lua_call_next(lua_State *state) {
    auto *next = static_cast<NextContext *>(lua_touserdata(state, lua_upvalueindex(1)));
    if (next != nullptr) {
        (void)native_call_next(next);
    }
    return 0;
}

void DreamModRuntime::invoke_chain(DispatchState &dispatch, size_t index) {
    if (index >= dispatch.chain.size()) {
        if (dispatch.original != nullptr) {
            dispatch.original(dispatch.original_context, *dispatch.context);
            dispatch.original_called = true;
        }
        return;
    }

    const HookEntry &entry = dispatch.chain[index];
    NextContext next{ &dispatch, index + 1, nullptr, LUA_NOREF };
    DreamModHookInvocationV1 invocation{ &next, &DreamModRuntime::native_call_next };

    if (entry.native_callback != nullptr) {
        entry.native_callback(entry.native_context, dispatch.context, &invocation);
        return;
    }
    if (entry.lua == nullptr || entry.lua_ref == LUA_NOREF) {
        invoke_chain(dispatch, index + 1);
        return;
    }

    lua_State *state = entry.lua;
    lua_rawgeti(state, LUA_REGISTRYINDEX, entry.lua_ref);
    push_dictionary(state, *dispatch.context);
    lua_pushvalue(state, -1);
    next.lua = state;
    next.table_ref = luaL_ref(state, LUA_REGISTRYINDEX);
    lua_pushlightuserdata(state, &next);
    lua_pushcclosure(state, &DreamModRuntime::lua_call_next, 1);
    if (lua_pcall(state, 2, 0, 0) != LUA_OK) {
        const char *error = lua_tostring(state, -1);
        ERR_PRINT(String("DreamMods Lua hook error (") + entry.owner + " / " + entry.point + "): " +
                (error != nullptr ? String::utf8(error) : String("unknown Lua error")));
        lua_pop(state, 1);
        ModInstance *mod = find_mod(entry.owner);
        if (mod != nullptr) mod->enabled = false;
        if (next.call_count == 0) invoke_chain(dispatch, index + 1);
    } else {
        sync_lua_table_to_dictionary(state, next.table_ref, *dispatch.context);
    }
    if (next.table_ref != LUA_NOREF) luaL_unref(state, LUA_REGISTRYINDEX, next.table_ref);
}

void DreamModRuntime::dispatch_internal(
        const StringName &point, Dictionary &context,
        DreamModNativeOriginalV1 original, void *original_context) {
    if (point.is_empty()) {
        if (original != nullptr) {
            original(original_context, context);
        }
        return;
    }

    std::vector<HookEntry> snapshot;
    snapshot.reserve(hooks.size());
    for (const HookEntry &entry : hooks) {
        if (entry.point != point) {
            continue;
        }
        ModInstance *owner = find_mod(entry.owner);
        if (owner != nullptr && !owner->enabled) {
            continue;
        }
        snapshot.push_back(entry);
    }
    std::sort(snapshot.begin(), snapshot.end(), [](const HookEntry &left, const HookEntry &right) {
        if (left.priority != right.priority) {
            return left.priority > right.priority;
        }
        return left.order < right.order;
    });

    DispatchState dispatch;
    dispatch.runtime = this;
    dispatch.context = &context;
    dispatch.original = original;
    dispatch.original_context = original_context;
    for (const HookEntry &entry : snapshot) {
        if (entry.stage == HOOK_AROUND || entry.stage == HOOK_REPLACE) {
            dispatch.chain.push_back(entry);
        }
    }

    ++dispatch_depth;
    for (const HookEntry &entry : snapshot) {
        if (entry.stage == HOOK_BEFORE) {
            if (entry.native_callback != nullptr) {
                DreamModHookInvocationV1 invocation{};
                entry.native_callback(entry.native_context, &context, &invocation);
            } else if (entry.lua != nullptr && entry.lua_ref != LUA_NOREF) {
                lua_rawgeti(entry.lua, LUA_REGISTRYINDEX, entry.lua_ref);
                push_dictionary(entry.lua, context);
                lua_pushvalue(entry.lua, -1);
                const int table_ref = luaL_ref(entry.lua, LUA_REGISTRYINDEX);
                if (lua_pcall(entry.lua, 1, 0, 0) != LUA_OK) {
                    const char *error = lua_tostring(entry.lua, -1);
                    ERR_PRINT(String("DreamMods Lua Before hook error: ") +
                            (error != nullptr ? String::utf8(error) : String("unknown Lua error")));
                    lua_pop(entry.lua, 1);
                    ModInstance *mod = find_mod(entry.owner);
                    if (mod != nullptr) mod->enabled = false;
                } else {
                    sync_lua_table_to_dictionary(entry.lua, table_ref, context);
                }
                luaL_unref(entry.lua, LUA_REGISTRYINDEX, table_ref);
            }
        }
    }

    invoke_chain(dispatch, 0);

    for (auto it = snapshot.rbegin(); it != snapshot.rend(); ++it) {
        if (it->stage != HOOK_AFTER) {
            continue;
        }
        if (it->native_callback != nullptr) {
            DreamModHookInvocationV1 invocation{};
            it->native_callback(it->native_context, &context, &invocation);
        } else if (it->lua != nullptr && it->lua_ref != LUA_NOREF) {
            lua_rawgeti(it->lua, LUA_REGISTRYINDEX, it->lua_ref);
            push_dictionary(it->lua, context);
            lua_pushvalue(it->lua, -1);
            const int table_ref = luaL_ref(it->lua, LUA_REGISTRYINDEX);
            if (lua_pcall(it->lua, 1, 0, 0) != LUA_OK) {
                const char *error = lua_tostring(it->lua, -1);
                ERR_PRINT(String("DreamMods Lua After hook error: ") +
                        (error != nullptr ? String::utf8(error) : String("unknown Lua error")));
                lua_pop(it->lua, 1);
                ModInstance *mod = find_mod(it->owner);
                if (mod != nullptr) mod->enabled = false;
            } else {
                sync_lua_table_to_dictionary(it->lua, table_ref, context);
            }
            luaL_unref(it->lua, LUA_REGISTRYINDEX, table_ref);
        }
    }

    --dispatch_depth;
    if (dispatch_depth == 0) {
        process_pending_unloads();
    }
}

void DreamModRuntime::dispatch_hook(
        const StringName &point, Dictionary &context,
        DreamModNativeOriginalV1 original, void *original_context) {
    if (singleton == nullptr) {
        if (original != nullptr) {
            original(original_context, context);
        }
        return;
    }
    singleton->dispatch_internal(point, context, original, original_context);
}

void DreamModRuntime::process_pending_unloads() {
    if (dispatch_depth != 0 || pending_unloads.empty()) {
        return;
    }
    std::vector<String> pending = std::move(pending_unloads);
    pending_unloads.clear();
    for (const String &id : pending) {
        unload_mod(id);
    }
}

namespace {
Object *dream_lua_get_object(lua_State *state, int index) {
    if (!lua_isinteger(state, index)) return nullptr;
    const int64_t id = static_cast<int64_t>(lua_tointeger(state, index));
    if (id <= 0) return nullptr;
    return ObjectDB::get_instance(ObjectID(static_cast<uint64_t>(id)));
}
} // namespace

int DreamModRuntime::lua_object_is_valid(lua_State *state) {
    lua_pushboolean(state, dream_lua_get_object(state, 1) != nullptr);
    return 1;
}

int DreamModRuntime::lua_object_get(lua_State *state) {
    Object *object = dream_lua_get_object(state, 1);
    const char *property = luaL_checkstring(state, 2);
    if (object == nullptr || property == nullptr) {
        lua_pushnil(state);
        return 1;
    }
    bool valid = false;
    Variant value = object->get(StringName(String::utf8(property)), &valid);
    if (!valid) {
        lua_pushnil(state);
    } else {
        push_variant(state, value);
    }
    return 1;
}

int DreamModRuntime::lua_object_set(lua_State *state) {
    Object *object = dream_lua_get_object(state, 1);
    const char *property = luaL_checkstring(state, 2);
    Variant value;
    if (object == nullptr || property == nullptr || !read_variant(state, 3, value)) {
        lua_pushboolean(state, false);
        return 1;
    }
    bool valid = false;
    object->set(StringName(String::utf8(property)), value, &valid);
    lua_pushboolean(state, valid);
    return 1;
}

int DreamModRuntime::lua_object_call(lua_State *state) {
    Object *object = dream_lua_get_object(state, 1);
    const char *method = luaL_checkstring(state, 2);
    if (object == nullptr || method == nullptr) {
        lua_pushnil(state);
        return 1;
    }
    Array arguments;
    for (int i = 3; i <= lua_gettop(state); ++i) {
        Variant value;
        if (!read_variant(state, i, value)) {
            lua_pushnil(state);
            return 1;
        }
        arguments.push_back(value);
    }
    Variant result = object->callv(StringName(String::utf8(method)), arguments);
    push_variant(state, result);
    return 1;
}

bool DreamModRuntime::load_mod(const String &package_path) {
    Ref<FileAccess> package_file = FileAccess::open(package_path, FileAccess::READ);
    if (package_file.is_null() || package_file->get_length() <= 0 ||
            package_file->get_length() > 32 * 1024 * 1024) {
        ERR_PRINT("DreamMods: package missing or larger than 32 MiB.");
        return false;
    }

    Ref<ZIPReader> zip;
    zip.instantiate();
    if (zip->open(package_path) != OK) {
        ERR_PRINT("DreamMods: invalid .dmod ZIP archive.");
        return false;
    }

    PackedByteArray manifest_bytes = zip->read_file("manifest.json", true, 64 * 1024);
    if (manifest_bytes.is_empty()) {
        zip->close();
        ERR_PRINT("DreamMods: manifest.json is missing or larger than 64 KiB.");
        return false;
    }
    String manifest_text = String::utf8(
            reinterpret_cast<const char *>(manifest_bytes.ptr()), manifest_bytes.size());
    Variant parsed = JSON::parse_string(manifest_text);
    if (parsed.get_type() != Variant::DICTIONARY) {
        zip->close();
        ERR_PRINT("DreamMods: manifest.json must contain a JSON object.");
        return false;
    }

    Dictionary manifest = parsed;
    String id = manifest.get("id", String());
    String name = manifest.get("name", id);
    String runtime = manifest.get("runtime", String("lua"));
    const bool needs_lua = runtime == "lua" || runtime == "hybrid";
    const bool needs_native = runtime == "native" || runtime == "hybrid";
    String entry_script = manifest.get("entry_script", String("scripts/main.lua"));
    String native_library = manifest.get("native_library", String());

    if (!valid_mod_id(id) || find_mod(id) != nullptr ||
            (!needs_lua && !needs_native) ||
            (needs_lua && !valid_package_path(entry_script)) ||
            (needs_native && !valid_package_path(native_library))) {
        zip->close();
        ERR_PRINT("DreamMods: invalid or duplicate manifest/runtime paths.");
        return false;
    }

    PackedByteArray source;
    if (needs_lua) {
        source = zip->read_file(entry_script, true, 4 * 1024 * 1024);
        if (source.is_empty()) {
            zip->close();
            ERR_PRINT("DreamMods: entry script missing or larger than 4 MiB.");
            return false;
        }
    }

    PackedByteArray native_bytes;
    if (needs_native) {
        native_bytes = zip->read_file(native_library, true, 16 * 1024 * 1024);
        if (native_bytes.is_empty()) {
            zip->close();
            ERR_PRINT("DreamMods: native library missing or larger than 16 MiB for this platform.");
            return false;
        }
    }
    zip->close();

    std::unique_ptr<ModInstance> mod = std::make_unique<ModInstance>();
    mod->id = id;
    mod->name = name;
    mod->package_path = package_path;
    ModInstance *active_mod = mod.get();
    mods.push_back(std::move(mod));

    if (needs_native) {
        if (DirAccess::make_dir_recursive("user://dream_mod_cache") != OK) {
            ERR_PRINT("DreamMods: cannot create native module cache directory.");
            unload_mod(id);
            return false;
        }

        String suffix = ".so";
#if defined(WINDOWS_ENABLED)
        suffix = ".dll";
#elif defined(MACOS_ENABLED)
        suffix = ".dylib";
#endif
        const String cached_library_path = "user://dream_mod_cache/" + id + suffix;
        Ref<FileAccess> output = FileAccess::open(cached_library_path, FileAccess::WRITE);
        if (output.is_null() || !output->store_buffer(native_bytes.ptr(), native_bytes.size())) {
            ERR_PRINT("DreamMods: failed to write native module to the private cache.");
            unload_mod(id);
            return false;
        }
        output->flush();
        output = Ref<FileAccess>();

#if defined(UNIX_ENABLED)
        const BitField<FileAccess::UnixPermissionFlags> readonly_permissions =
                FileAccess::UNIX_READ_OWNER | FileAccess::UNIX_READ_GROUP | FileAccess::UNIX_READ_OTHER;
        if (FileAccess::set_unix_permissions(cached_library_path, readonly_permissions) != OK) {
            ERR_PRINT("DreamMods: could not mark the extracted native library read-only.");
            unload_mod(id);
            return false;
        }
#endif

        const String absolute_library_path = ProjectSettings::get_singleton()->globalize_path(cached_library_path);
        if (OS::get_singleton()->open_dynamic_library(
                    absolute_library_path, active_mod->native_library_handle) != OK) {
            ERR_PRINT("DreamMods: the native library could not be loaded for this OS/ABI.");
            unload_mod(id);
            return false;
        }

        void *entry_symbol = nullptr;
        if (OS::get_singleton()->get_dynamic_library_symbol_handle(
                    active_mod->native_library_handle, "dream_dmod_entry_v1", entry_symbol) != OK ||
                entry_symbol == nullptr) {
            ERR_PRINT("DreamMods: native module is missing dream_dmod_entry_v1.");
            unload_mod(id);
            return false;
        }

        NativeModEntryPointV1 entry_point = reinterpret_cast<NativeModEntryPointV1>(entry_symbol);
        active_mod->native_exports = entry_point();
        if (active_mod->native_exports == nullptr ||
                active_mod->native_exports->struct_size < sizeof(NativeModExportsV1) ||
                active_mod->native_exports->abi_version != NATIVE_ABI_V1 ||
                active_mod->native_exports->mod_id == nullptr ||
                String::utf8(active_mod->native_exports->mod_id) != id ||
                active_mod->native_exports->create == nullptr ||
                active_mod->native_exports->enable == nullptr ||
                active_mod->native_exports->unload == nullptr) {
            ERR_PRINT("DreamMods: native module descriptor or identity is invalid.");
            active_mod->native_exports = nullptr;
            unload_mod(id);
            return false;
        }

        active_mod->native_api = get_native_host_api();
        active_mod->native_context = active_mod->native_exports->create(&active_mod->native_api);
        if (active_mod->native_context == nullptr) {
            ERR_PRINT("DreamMods: native module create callback failed.");
            unload_mod(id);
            return false;
        }
    }

    if (needs_lua) {
        active_mod->state = luaL_newstate();
        if (active_mod->state == nullptr) {
            ERR_PRINT("DreamMods: Lua VM allocation failed.");
            unload_mod(id);
            return false;
        }

        luaL_openlibs(active_mod->state);
        lua_newtable(active_mod->state);
        lua_newtable(active_mod->state);
        lua_pushcfunction(active_mod->state, &DreamModRuntime::lua_register_hook);
        lua_setfield(active_mod->state, -2, "register");
        lua_setfield(active_mod->state, -2, "hooks");

        lua_newtable(active_mod->state);
        lua_pushcfunction(active_mod->state, &DreamModRuntime::lua_object_is_valid);
        lua_setfield(active_mod->state, -2, "is_valid");
        lua_pushcfunction(active_mod->state, &DreamModRuntime::lua_object_get);
        lua_setfield(active_mod->state, -2, "get");
        lua_pushcfunction(active_mod->state, &DreamModRuntime::lua_object_set);
        lua_setfield(active_mod->state, -2, "set");
        lua_pushcfunction(active_mod->state, &DreamModRuntime::lua_object_call);
        lua_setfield(active_mod->state, -2, "call");
        lua_setfield(active_mod->state, -2, "objects");
        lua_setglobal(active_mod->state, "dream");

        CharString chunk_name = (String("@") + id + "/" + entry_script).utf8();
        if (luaL_loadbufferx(active_mod->state, reinterpret_cast<const char *>(source.ptr()), source.size(),
                    chunk_name.get_data(), "t") != LUA_OK ||
                lua_pcall(active_mod->state, 0, 0, 0) != LUA_OK) {
            const char *error = lua_tostring(active_mod->state, -1);
            ERR_PRINT(String("DreamMods: Lua load failed for ") + id + ": " +
                    (error != nullptr ? String::utf8(error) : String("unknown error")));
            lua_pop(active_mod->state, 1);
            unload_mod(id);
            return false;
        }
    }

    if (needs_native && active_mod->native_exports->enable(active_mod->native_context) == 0) {
        ERR_PRINT("DreamMods: native module enable callback failed.");
        unload_mod(id);
        return false;
    }
    active_mod->enabled = true;
    return true;
}

bool DreamModRuntime::unload_mod(const String &mod_id) {
    ModInstance *mod = find_mod(mod_id);
    if (mod == nullptr) return false;
    if (dispatch_depth != 0) {
        mod->enabled = false;
        for (const String &pending : pending_unloads) {
            if (pending == mod_id) return true;
        }
        pending_unloads.push_back(mod_id);
        return true;
    }

    if (mod->enabled && mod->native_exports != nullptr && mod->native_exports->disable != nullptr) {
        mod->native_exports->disable(mod->native_context);
    }
    mod->enabled = false;

    // Unregister every callback before the native library or Lua state is destroyed.
    for (auto it = hooks.begin(); it != hooks.end();) {
        if (it->owner == mod_id) {
            if (it->lua != nullptr && it->lua_ref != LUA_NOREF) {
                luaL_unref(it->lua, LUA_REGISTRYINDEX, it->lua_ref);
            }
            it = hooks.erase(it);
        } else {
            ++it;
        }
    }

    if (mod->native_exports != nullptr && mod->native_context != nullptr &&
            mod->native_exports->unload != nullptr) {
        mod->native_exports->unload(mod->native_context);
        mod->native_context = nullptr;
    }
    if (mod->native_library_handle != nullptr) {
        OS::get_singleton()->close_dynamic_library(mod->native_library_handle);
        mod->native_library_handle = nullptr;
    }
    mod->native_exports = nullptr;

    for (auto it = mods.begin(); it != mods.end(); ++it) {
        if ((*it)->id == mod_id) {
            mods.erase(it);
            return true;
        }
    }
    return false;
}

bool DreamModRuntime::enable_mod(const String &mod_id) {
    ModInstance *mod = find_mod(mod_id);
    if (mod == nullptr || mod->enabled) return false;
    if (mod->native_exports != nullptr && mod->native_exports->enable != nullptr &&
            mod->native_exports->enable(mod->native_context) == 0) {
        return false;
    }
    mod->enabled = true;
    return true;
}

bool DreamModRuntime::disable_mod(const String &mod_id) {
    ModInstance *mod = find_mod(mod_id);
    if (mod == nullptr || !mod->enabled) return false;
    if (mod->native_exports != nullptr && mod->native_exports->disable != nullptr) {
        mod->native_exports->disable(mod->native_context);
    }
    mod->enabled = false;
    return true;
}

void DreamModRuntime::update_mods(double delta) {
    if (!std::isfinite(delta) || delta < 0.0) return;
    const size_t count = mods.size();
    for (size_t i = 0; i < count && i < mods.size(); ++i) {
        ModInstance *mod = mods[i].get();
        if (!mod->enabled) continue;
        if (mod->native_exports != nullptr && mod->native_exports->update != nullptr) {
            mod->native_exports->update(mod->native_context, delta);
        }
        if (mod->state != nullptr) {
            lua_getglobal(mod->state, "on_update");
            if (lua_isfunction(mod->state, -1)) {
                lua_pushnumber(mod->state, static_cast<lua_Number>(delta));
                if (lua_pcall(mod->state, 1, 0, 0) != LUA_OK) {
                    const char *error = lua_tostring(mod->state, -1);
                    ERR_PRINT(String("DreamMods Lua on_update error (") + mod->id + "): " +
                            (error != nullptr ? String::utf8(error) : String("unknown Lua error")));
                    lua_pop(mod->state, 1);
                    mod->enabled = false;
                }
            } else {
                lua_pop(mod->state, 1);
            }
        }
    }
}

PackedStringArray DreamModRuntime::get_loaded_mods() const {
    PackedStringArray result;
    for (const std::unique_ptr<ModInstance> &mod : mods) {
        result.push_back(mod->id);
    }
    return result;
}

int32_t DreamModRuntime::get_hook_count() const {
    return static_cast<int32_t>(hooks.size());
}

void DreamModRuntime::_bind_methods() {
    ClassDB::bind_method(D_METHOD("load_mod", "package_path"), &DreamModRuntime::load_mod);
    ClassDB::bind_method(D_METHOD("unload_mod", "mod_id"), &DreamModRuntime::unload_mod);
    ClassDB::bind_method(D_METHOD("enable_mod", "mod_id"), &DreamModRuntime::enable_mod);
    ClassDB::bind_method(D_METHOD("disable_mod", "mod_id"), &DreamModRuntime::disable_mod);
    ClassDB::bind_method(D_METHOD("get_loaded_mods"), &DreamModRuntime::get_loaded_mods);
    ClassDB::bind_method(D_METHOD("get_hook_count"), &DreamModRuntime::get_hook_count);
}
