#include "register_types.h"

#include "dream_runtime.h"

#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

void initialize_dream_runtime(ModuleInitializationLevel level) {
    if (level != MODULE_INITIALIZATION_LEVEL_SCENE) {
        return;
    }

    ClassDB::register_class<DreamRuntimeProbe>();
    UtilityFunctions::print("[DreamRuntime] initialized");
}

void uninitialize_dream_runtime(ModuleInitializationLevel level) {
    if (level != MODULE_INITIALIZATION_LEVEL_SCENE) {
        return;
    }

    UtilityFunctions::print("[DreamRuntime] shutdown");
}
