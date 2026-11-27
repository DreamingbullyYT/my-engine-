#pragma once

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/string.hpp>

namespace godot {

class DreamRuntimeProbe : public Node {
    GDCLASS(DreamRuntimeProbe, Node)

protected:
    static void _bind_methods();

public:
    String get_probe_message() const;
    int64_t add_numbers(int64_t left, int64_t right) const;
};

} // namespace godot
