#include <dream/dmod/native_mod_api.hpp>
#include <cstdint>
#include <new>
namespace {
struct State { std::int32_t* counter{}; };
void* create_mod(const dream::dmod::NativeModHostApiV1* host,
                 const dream::dmod::NativeModInfoV1* info) noexcept {
    if (!host || !info || !info->id || !host->query_interface) return nullptr;
    auto* counter = static_cast<std::int32_t*>(
        host->query_interface(host->host_context, "dream.test.counter", 1));
    if (!counter) return nullptr;
    ++(*counter);
    return new (std::nothrow) State{counter};
}
std::uint8_t enable_mod(void* p) noexcept {
    if (!p) return 0;
    *static_cast<State*>(p)->counter *= 10;
    return 1;
}
void update_mod(void* p, double) noexcept {
    if (p) *static_cast<State*>(p)->counter += 100;
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
