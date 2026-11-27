#include "register_types.h"

#include "core/config/engine.h"
#include "core/object/class_db.h"
#include "core/os/memory.h"
#include "dream_mod_runtime.h"

static DreamModRuntime *dream_mod_runtime_singleton = nullptr;

void initialize_dream_engine_module(ModuleInitializationLevel p_level) {
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
        return;
    }

    GDREGISTER_CLASS(DreamModRuntime);
    dream_mod_runtime_singleton = memnew(DreamModRuntime);
    DreamModRuntime::set_singleton(dream_mod_runtime_singleton);
    Engine::get_singleton()->add_singleton(Engine::Singleton("DreamMods", dream_mod_runtime_singleton));
}

void uninitialize_dream_engine_module(ModuleInitializationLevel p_level) {
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE || dream_mod_runtime_singleton == nullptr) {
        return;
    }

    Engine::get_singleton()->remove_singleton("DreamMods");
    DreamModRuntime::set_singleton(nullptr);
    memdelete(dream_mod_runtime_singleton);
    dream_mod_runtime_singleton = nullptr;
}
