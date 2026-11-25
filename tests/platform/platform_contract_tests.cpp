#include <dream/platform/filesystem.hpp>
#include <dream/platform/input.hpp>
#include <dream/platform/window.hpp>

#include <cstdlib>
#include <iostream>
#include <string_view>

namespace {

void require(bool condition, std::string_view message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << std::endl;
        std::exit(EXIT_FAILURE);
    }
}

} // namespace

int main() {
    using namespace dream::platform;

    require(is_valid_virtual_path("assets/textures/stone.png"),
            "nested relative virtual path accepted");
    require(is_valid_virtual_path("save_01/world.dat"),
            "ordinary save path accepted");
    require(!is_valid_virtual_path(""), "empty path rejected");
    require(!is_valid_virtual_path("/etc/passwd"), "absolute path rejected");
    require(!is_valid_virtual_path("../outside"), "parent traversal rejected");
    require(!is_valid_virtual_path("assets/../outside"), "nested traversal rejected");
    require(!is_valid_virtual_path("assets//file"), "empty path segment rejected");
    require(!is_valid_virtual_path("assets/./file"), "dot segment rejected");
    require(!is_valid_virtual_path("C:/outside"), "drive-prefixed path rejected");
    require(!is_valid_virtual_path("assets\\file"), "backslash path rejected");
    require(!is_valid_virtual_path("assets/"), "trailing separator rejected");

    const WindowId first{1};
    const WindowId second{2};
    require(static_cast<bool>(first), "nonzero window ID is valid");
    require(!static_cast<bool>(WindowId{}), "zero window ID is invalid");
    require(first != second, "window IDs compare by value");

    WindowConfig config;
    require(config.width > 0 && config.height > 0, "window has a nonzero default size");
    require(config.high_dpi, "high DPI enabled by default");

    InputEvent touch;
    touch.type = InputEventType::PointerDown;
    touch.pointer_kind = PointerKind::Touch;
    touch.pointer_id = 42;
    touch.x = 100.0F;
    touch.y = 200.0F;
    require(touch.pointer_kind == PointerKind::Touch && touch.pointer_id == 42,
            "touch input carries independent pointer identity");

    InputEvent axis;
    axis.type = InputEventType::GamepadAxis;
    axis.axis_value = -1.0F;
    require(axis.axis_value >= -1.0F && axis.axis_value <= 1.0F,
            "gamepad axis representation supports normalized values");

    std::cout << "Dream Engine platform contract tests passed." << std::endl;
    return EXIT_SUCCESS;
}
