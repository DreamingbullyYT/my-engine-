#pragma once

#include <cstdint>

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

    // Key values are backend-neutral platform key codes. Text arrives separately as Unicode.
    std::int32_t key_code{0};
    char32_t codepoint{U'\0'};
    InputModifiers modifiers{};

    PointerKind pointer_kind{PointerKind::Unknown};
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
