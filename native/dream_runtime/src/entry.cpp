#include "register_types.h"

#include <godot_cpp/godot.hpp>

using namespace godot;

extern "C" {

GDExtensionBool GDE_EXPORT dream_runtime_library_init(
    GDExtensionInterfaceGetProcAddress get_proc_address,
    GDExtensionClassLibraryPtr library,
    GDExtensionInitialization *initialization
) {
    GDExtensionBinding::InitObject init_object(
        get_proc_address,
        library,
        initialization
    );

    init_object.register_initializer(initialize_dream_runtime);
    init_object.register_terminator(uninitialize_dream_runtime);
    init_object.set_minimum_library_initialization_level(
        MODULE_INITIALIZATION_LEVEL_SCENE
    );

    return init_object.init();
}

} // extern "C"
