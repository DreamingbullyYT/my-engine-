#pragma once

#include <cstdint>
#include <string>

namespace dream::platform {

enum class InputEventType : std::uint8_t {
    KeyDown,
    KeyUp,
    TextInput,
    PointerMove,
    PointerDown,
    PointerUp,
    PointerCancel,
    Scroll,
    GamepadConnected,
    GamepadDisconnected,
    GamepadButtonDown,
    GamepadButtonUp,
    GamepadAxis
};

enum class PointerKind : std::uint8_t {
    Mouse,
    Touch,
    Pen,
    Unknown
};

struct InputModifiers final {
    bool shift{false};
    bool control{false};
    bool alt{false};
    bool super{false};
};

struct InputEvent final {
    InputEventType type{InputEventType::PointerMove};
    std::uint64_t timestamp_ns{0};
    std::uint64_t device_id{0};
    std::uint64_t pointer_id{0};

    // Keyboard values use USB HID Keyboard/Keypad usage IDs, not OS virtual-key codes.
    std::uint16_t key_code{0};
    char32_t codepoint{U'\0'};
    // GameActivity IME snapshots use Modified UTF-8 bytes and carry the raw
    // GameTextInput selection/composing span offsets. The buffer is owned.
    std::string text_utf8;
    bool has_text_snapshot{false};
    std::int32_t selection_start{-1};
    std::int32_t selection_end{-1};
    std::int32_t composing_start{-1};
    std::int32_t composing_end{-1};
    InputModifiers modifiers{};

    PointerKind pointer_kind{PointerKind::Unknown};
    // Pointer positions use logical window pixels; deltas on Scroll are scroll units.
    float x{0.0F};
    float y{0.0F};
    float delta_x{0.0F};
    float delta_y{0.0F};
    std::uint8_t button{0};

    // Gamepad axes use the normalized range [-1, 1].
    std::uint16_t gamepad_control{0};
    float axis_value{0.0F};
};

class IInputSystem {
public:
    virtual ~IInputSystem() = default;
    [[nodiscard]] virtual bool poll_input_event(InputEvent& event) = 0;
};

} // namespace dream::platform
