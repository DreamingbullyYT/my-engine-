#include <dream/dmod/native_mod_api.hpp>
#include <dream/dmod/native_mod_manager.hpp>
#include <cstdint>
#include <array>
#include <new>
namespace {
struct State {
    std::int32_t* counter{};
    bool* unload_on_update{};
    dream::dmod::NativeModManager* manager{};
    std::array<dream::dmod::NativeHookRegistrationId, 4> hooks{};
};

void hook_before(void*, dream::dmod::NativeHookInvocationV1* invocation) noexcept {
    if (!invocation || !invocation->user_data) return;
    *static_cast<std::int32_t*>(invocation->user_data) += 5;
}

void hook_around(void*, dream::dmod::NativeHookInvocationV1* invocation) noexcept {
    if (!invocation || !invocation->user_data || !invocation->call_next) return;
    if (invocation->call_next(invocation->next_context) != 0)
        *static_cast<std::int32_t*>(invocation->user_data) += 3;
}

void hook_after(void*, dream::dmod::NativeHookInvocationV1* invocation) noexcept {
    if (!invocation || !invocation->user_data) return;
    *static_cast<std::int32_t*>(invocation->user_data) += 7;
}

void hook_replace(void*, dream::dmod::NativeHookInvocationV1* invocation) noexcept {
    if (!invocation || !invocation->user_data) return;
    *static_cast<std::int32_t*>(invocation->user_data) = 77;
}
void* create_mod(const dream::dmod::NativeModHostApiV1* host,
                 const dream::dmod::NativeModInfoV1* info) noexcept {
    if (!host || !info || !info->id || !host->query_interface) return nullptr;
    auto* counter = static_cast<std::int32_t*>(
        host->query_interface(host->host_context, "dream.test.counter", 1));
    auto* unload_on_update = static_cast<bool*>(
        host->query_interface(host->host_context, "dream.test.unload_on_update", 1));
    auto* manager = static_cast<dream::dmod::NativeModManager*>(
        host->query_interface(host->host_context, "dream.test.manager", 1));
    if (!counter || !unload_on_update || !manager) return nullptr;
    ++(*counter);
    return new (std::nothrow) State{counter, unload_on_update, manager};
}
std::uint8_t enable_mod(void* p) noexcept {
    if (!p) return 0;
    auto* state = static_cast<State*>(p);
    *state->counter *= 10;
    if (state->hooks[0] == 0) {
        auto& hooks = state->manager->hook_registry();
        state->hooks[0] = hooks.register_hook(
            "test.native", "test.game.adjust", dream::dmod::NativeHookStage::Before,
            10, &hook_before, state);
        state->hooks[1] = hooks.register_hook(
            "test.native", "test.game.adjust", dream::dmod::NativeHookStage::Around,
            10, &hook_around, state);
        state->hooks[2] = hooks.register_hook(
            "test.native", "test.game.adjust", dream::dmod::NativeHookStage::After,
            10, &hook_after, state);
        state->hooks[3] = hooks.register_hook(
            "test.native", "test.game.replace", dream::dmod::NativeHookStage::Replace,
            10, &hook_replace, state);
        for (const auto registration : state->hooks)
            if (registration == 0) return 0;
    }
    return 1;
}
void update_mod(void* p, double) noexcept {
    if (!p) return;
    auto* state = static_cast<State*>(p);
    *state->counter += 100;
    if (*state->unload_on_update) {
        *state->unload_on_update = false;
        *state->counter += 1;
        (void)state->manager->unload("test.native");
    }
}
void disable_mod(void* p) noexcept {
    if (p) *static_cast<State*>(p)->counter += 1000;
}
void destroy_mod(void* p) noexcept {
    if (!p) return;
    auto* state = static_cast<State*>(p);
    *state->counter += 10000;
    delete state;
}
const dream::dmod::NativeModExportsV1 exports{
    static_cast<std::uint32_t>(sizeof(dream::dmod::NativeModExportsV1)),
    dream::dmod::native_mod_abi_v1, "test.native", "Native Test Mod", 1, 2, 3,
    &create_mod, &enable_mod, &update_mod, &disable_mod, &destroy_mod
};
}
DREAM_DMOD_EXPORT const dream::dmod::NativeModExportsV1* dream_mod_entry_v1() noexcept {
    return &exports;
}
