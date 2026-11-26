#include <dream/input/action_map.hpp>

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string_view>

namespace {
using dream::input::ActionMap;
using dream::input::ActionMapError;
using dream::platform::InputEvent;
using dream::platform::InputEventType;
using dream::platform::PointerKind;

void require(bool condition, std::string_view message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

InputEvent key_event(InputEventType type, std::uint16_t code,
                     bool control = false, std::uint64_t timestamp = 1) {
    InputEvent event;
    event.type = type;
    event.key_code = code;
    event.modifiers.control = control;
    event.timestamp_ns = timestamp;
    return event;
}
}

int main() {
    ActionMap actions;
    require(actions.add_action("jump") == ActionMapError::None,
            "jump action registers");
    require(actions.add_action("fire") == ActionMapError::None,
            "fire action registers");
    require(actions.add_action("move/horizontal") == ActionMapError::None,
            "hierarchical action name registers");
    require(actions.add_action("save") == ActionMapError::None,
            "save action registers");
    require(actions.add_action("jump") == ActionMapError::DuplicateAction,
            "duplicate action names are rejected");
    require(actions.add_action("bad action") == ActionMapError::InvalidName,
            "nonportable action names are rejected");

    constexpr std::uint16_t hid_space = 0x2CU;
    constexpr std::uint16_t hid_s = 0x16U;
    require(actions.bind_key("jump", hid_space) == ActionMapError::None,
            "space binds to jump");
    require(actions.bind_gamepad_button("jump", 1U, 42U) == ActionMapError::None,
            "gamepad button also binds to jump");
    require(actions.bind_mouse_button("fire", 1U) == ActionMapError::None,
            "mouse left button binds to fire");
    require(actions.bind_gamepad_axis(
                "move/horizontal", 0U + 1U, 42U, 0.2F, -1.0F) ==
                ActionMapError::None,
            "gamepad axis binds with deadzone and inverted scale");
    require(actions.bind_key(
                "save", hid_s, dream::platform::InputModifiers{.control = true}) ==
                ActionMapError::None,
            "Ctrl+S binds to save");
    require(actions.bind_key("missing", hid_s) == ActionMapError::UnknownAction,
            "unknown action binding is rejected");
    require(actions.bind_gamepad_axis(
                "move/horizontal", 2U, 42U, 1.0F) ==
                ActionMapError::InvalidBinding,
            "invalid deadzone is rejected");
    require(actions.bind_gamepad_axis(
                "move/horizontal", 2U, 42U, 0.1F,
                std::numeric_limits<float>::quiet_NaN()) ==
                ActionMapError::InvalidBinding,
            "non-finite binding scale is rejected");

    auto jump = key_event(InputEventType::KeyDown, hid_space, false, 100U);
    require(actions.process_event(jump), "space keydown is handled");
    auto jump_state = actions.state("jump");
    require(jump_state && jump_state->down && jump_state->just_pressed &&
            jump_state->value == 1.0F && jump_state->timestamp_ns == 100U,
            "digital keydown activates the action and press edge");

    require(actions.process_event(jump), "key repeat remains a handled event");
    jump_state = actions.state("jump");
    require(jump_state && jump_state->just_pressed,
            "key repeat does not clear the press edge");

    auto gamepad_jump = jump;
    gamepad_jump.type = InputEventType::GamepadButtonDown;
    gamepad_jump.device_id = 42U;
    gamepad_jump.gamepad_control = 1U;
    require(actions.process_event(gamepad_jump),
            "gamepad button activates a second jump binding");
    auto key_up = key_event(InputEventType::KeyUp, hid_space, false, 110U);
    require(actions.process_event(key_up), "space keyup is handled");
    jump_state = actions.state("jump");
    require(jump_state && jump_state->down && !jump_state->just_released,
            "releasing one of two bindings keeps action held");

    auto gamepad_up = gamepad_jump;
    gamepad_up.type = InputEventType::GamepadButtonUp;
    gamepad_up.timestamp_ns = 120U;
    require(actions.process_event(gamepad_up), "gamepad button release is handled");
    jump_state = actions.state("jump");
    require(jump_state && !jump_state->down && jump_state->just_released,
            "action releases after the final binding is released");

    actions.begin_frame();
    jump_state = actions.state("jump");
    require(jump_state && !jump_state->just_pressed && !jump_state->just_released,
            "frame boundary clears transient edges");

    auto wrong_modifier = key_event(InputEventType::KeyDown, hid_s, false, 130U);
    require(!actions.process_event(wrong_modifier),
            "Ctrl+S does not activate without the required modifier");
    auto correct_modifier = key_event(InputEventType::KeyDown, hid_s, true, 140U);
    require(actions.process_event(correct_modifier),
            "Ctrl+S activates with the exact modifier set");
    auto save_state = actions.state("save");
    require(save_state && save_state->down && save_state->just_pressed,
            "modifier key creates a press edge");
    auto modifier_released_first = key_event(InputEventType::KeyUp, hid_s, false, 150U);
    require(actions.process_event(modifier_released_first),
            "key release clears a held binding even if modifiers changed");
    save_state = actions.state("save");
    require(save_state && !save_state->down && save_state->just_released,
            "modifier action releases cleanly");

    InputEvent touch_down;
    touch_down.type = InputEventType::PointerDown;
    touch_down.pointer_kind = PointerKind::Touch;
    touch_down.button = 1U;
    require(!actions.process_event(touch_down),
            "touch input does not activate a mouse-only binding");

    InputEvent mouse_down;
    mouse_down.type = InputEventType::PointerDown;
    mouse_down.pointer_kind = PointerKind::Mouse;
    mouse_down.button = 1U;
    mouse_down.timestamp_ns = 200U;
    require(actions.process_event(mouse_down), "mouse button activates fire");
    auto fire_state = actions.state("fire");
    require(fire_state && fire_state->down && fire_state->value == 1.0F,
            "mouse binding updates action state");
    mouse_down.type = InputEventType::PointerCancel;
    require(actions.process_event(mouse_down), "pointer cancel releases mouse binding");
    fire_state = actions.state("fire");
    require(fire_state && !fire_state->down && fire_state->just_released,
            "pointer cancellation releases the action");

    InputEvent axis;
    axis.type = InputEventType::GamepadAxis;
    axis.device_id = 42U;
    axis.gamepad_control = 1U;
    axis.axis_value = 0.1F;
    axis.timestamp_ns = 300U;
    require(actions.process_event(axis), "gamepad axis event is handled");
    auto move_state = actions.state("move/horizontal");
    require(move_state && !move_state->down && move_state->value == 0.0F,
            "axis values inside the deadzone are neutral");

    axis.axis_value = 0.6F;
    axis.timestamp_ns = 310U;
    require(actions.process_event(axis), "axis movement is processed");
    move_state = actions.state("move/horizontal");
    require(move_state && move_state->down &&
            std::abs(move_state->value + 0.5F) < 0.0001F,
            "axis deadzone rescales and inverted scale is applied");

    InputEvent disconnected;
    disconnected.type = InputEventType::GamepadDisconnected;
    disconnected.device_id = 42U;
    disconnected.timestamp_ns = 320U;
    require(actions.process_event(disconnected),
            "controller disconnect resets mapped controls");
    move_state = actions.state("move/horizontal");
    require(move_state && !move_state->down && move_state->value == 0.0F &&
            move_state->just_released,
            "disconnect releases active analog actions");

    const auto names = actions.action_names();
    require(names.size() == 4U && names[0] == "jump" &&
            names[2] == "move/horizontal",
            "action names retain registration order");
    require(actions.remove_action("fire"), "action removal succeeds");
    require(!actions.state("fire") && actions.binding_count() == 4U,
            "removing an action also removes its bindings");
    require(!actions.remove_action("missing"), "unknown action removal fails");

    std::cout << "Dream Engine action map tests passed.\n";
    return EXIT_SUCCESS;
}
