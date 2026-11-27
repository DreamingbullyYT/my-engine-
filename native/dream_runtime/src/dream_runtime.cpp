#include "dream_runtime.h"

#include <godot_cpp/core/class_db.hpp>

namespace godot {

void DreamRuntimeProbe::_bind_methods() {
    ClassDB::bind_method(
        D_METHOD("get_probe_message"),
        &DreamRuntimeProbe::get_probe_message
    );
    ClassDB::bind_method(
        D_METHOD("add_numbers", "left", "right"),
        &DreamRuntimeProbe::add_numbers
    );
}

String DreamRuntimeProbe::get_probe_message() const {
    return "dream-runtime-gdextension-ok";
}

int64_t DreamRuntimeProbe::add_numbers(int64_t left, int64_t right) const {
    return left + right;
}

} // namespace godot
