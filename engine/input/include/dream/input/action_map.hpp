#pragma once

#include <dream/platform/input.hpp>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace dream::input {

struct ActionState final {
    bool down{false};
    bool just_pressed{false};
    bool just_released{false};
    float value{0.0F};
    std::uint64_t timestamp_ns{0};
};

enum class ActionMapError : std::uint8_t {
    None,
    InvalidName,
    DuplicateAction,
    UnknownAction,
    InvalidBinding
};

class ActionMap final {
public:
    ActionMap() = default;
    ActionMap(const ActionMap&) = delete;
    ActionMap& operator=(const ActionMap&) = delete;

    [[nodiscard]] ActionMapError add_action(std::string name) {
        if (!valid_name(name)) return ActionMapError::InvalidName;
        std::lock_guard lock(mutex_);
        if (action_indices_.contains(name)) return ActionMapError::DuplicateAction;
        const std::size_t index = actions_.size();
        actions_.push_back(Action{std::move(name), {}});
        action_indices_.emplace(actions_.back().name, index);
        return ActionMapError::None;
    }

    [[nodiscard]] bool remove_action(std::string_view name) {
        std::lock_guard lock(mutex_);
        const auto found = action_indices_.find(std::string{name});
        if (found == action_indices_.end()) return false;
        const std::size_t removed_index = found->second;
        actions_.erase(actions_.begin() + static_cast<std::ptrdiff_t>(removed_index));
        bindings_.erase(
            std::remove_if(bindings_.begin(), bindings_.end(),
                [removed_index](const Binding& binding) {
                    return binding.action_index == removed_index;
                }),
            bindings_.end());
        for (auto& binding : bindings_) {
            if (binding.action_index > removed_index) --binding.action_index;
        }
        action_indices_.clear();
        for (std::size_t index = 0; index < actions_.size(); ++index)
            action_indices_.emplace(actions_[index].name, index);
        return true;
    }

    [[nodiscard]] ActionMapError bind_key(
        std::string_view action, std::uint16_t hid_usage,
        platform::InputModifiers modifiers = {}, float scale = 1.0F) {
        if (hid_usage == 0U || !valid_scale(scale))
            return ActionMapError::InvalidBinding;
        return add_binding(action, Binding{
            .kind = BindingKind::Key,
            .control = hid_usage,
            .modifiers = modifiers,
            .scale = scale
        });
    }

    [[nodiscard]] ActionMapError bind_mouse_button(
        std::string_view action, std::uint8_t button, float scale = 1.0F) {
        if (button == 0U || !valid_scale(scale))
            return ActionMapError::InvalidBinding;
        return add_binding(action, Binding{
            .kind = BindingKind::MouseButton,
            .control = button,
            .scale = scale
        });
    }

    [[nodiscard]] ActionMapError bind_gamepad_button(
        std::string_view action, std::uint16_t control,
        std::uint64_t device_id, float scale = 1.0F) {
        if (control == 0U || !valid_scale(scale))
            return ActionMapError::InvalidBinding;
        return add_binding(action, Binding{
            .kind = BindingKind::GamepadButton,
            .control = control,
            .device_id = device_id,
            .scale = scale
        });
    }

    [[nodiscard]] ActionMapError bind_gamepad_axis(
        std::string_view action, std::uint16_t control,
        std::uint64_t device_id, float deadzone = 0.15F,
        float scale = 1.0F) {
        if (control == 0U || !valid_scale(scale) || !std::isfinite(deadzone) ||
            deadzone < 0.0F || deadzone >= 0.95F) {
            return ActionMapError::InvalidBinding;
        }
        return add_binding(action, Binding{
            .kind = BindingKind::GamepadAxis,
            .control = control,
            .device_id = device_id,
            .scale = scale,
            .deadzone = deadzone
        });
    }

    // Call once before polling events for a frame. State remains latched until
    // events update it; edge flags are reset here, not by reading the state.
    void begin_frame() {
        std::lock_guard lock(mutex_);
        for (auto& action : actions_) {
            action.state.just_pressed = false;
            action.state.just_released = false;
        }
    }

    [[nodiscard]] bool process_event(const platform::InputEvent& event) {
        std::lock_guard lock(mutex_);
        bool handled = false;
        std::vector<std::size_t> changed_actions;

        for (auto& binding : bindings_) {
            bool changed = false;
            switch (event.type) {
            case platform::InputEventType::KeyDown:
                if (binding.kind == BindingKind::Key &&
                    binding.control == event.key_code &&
                    modifiers_equal(binding.modifiers, event.modifiers)) {
                    if (!binding.active) {
                        binding.active = true;
                        binding.value = binding.scale;
                        changed = true;
                    }
                    handled = true;
                }
                break;

            case platform::InputEventType::KeyUp:
                if (binding.kind == BindingKind::Key &&
                    binding.control == event.key_code && binding.active) {
                    binding.active = false;
                    binding.value = 0.0F;
                    changed = true;
                    handled = true;
                }
                break;

            case platform::InputEventType::PointerDown:
                if (binding.kind == BindingKind::MouseButton &&
                    event.pointer_kind == platform::PointerKind::Mouse &&
                    binding.control == event.button) {
                    if (!binding.active) {
                        binding.active = true;
                        binding.value = binding.scale;
                        changed = true;
                    }
                    handled = true;
                }
                break;

            case platform::InputEventType::PointerUp:
            case platform::InputEventType::PointerCancel:
                if (binding.kind == BindingKind::MouseButton &&
                    event.pointer_kind == platform::PointerKind::Mouse &&
                    binding.control == event.button && binding.active) {
                    binding.active = false;
                    binding.value = 0.0F;
                    changed = true;
                    handled = true;
                }
                break;

            case platform::InputEventType::GamepadButtonDown:
                if (binding.kind == BindingKind::GamepadButton &&
                    binding.control == event.gamepad_control &&
                    binding.device_id == event.device_id) {
                    if (!binding.active) {
                        binding.active = true;
                        binding.value = binding.scale;
                        changed = true;
                    }
                    handled = true;
                }
                break;

            case platform::InputEventType::GamepadButtonUp:
                if (binding.kind == BindingKind::GamepadButton &&
                    binding.control == event.gamepad_control &&
                    binding.device_id == event.device_id && binding.active) {
                    binding.active = false;
                    binding.value = 0.0F;
                    changed = true;
                    handled = true;
                }
                break;

            case platform::InputEventType::GamepadAxis:
                if (binding.kind == BindingKind::GamepadAxis &&
                    binding.control == event.gamepad_control &&
                    binding.device_id == event.device_id) {
                    const float raw = std::isfinite(event.axis_value)
                        ? std::clamp(event.axis_value, -1.0F, 1.0F)
                        : 0.0F;
                    float normalized = 0.0F;
                    const float magnitude = std::abs(raw);
                    if (magnitude > binding.deadzone) {
                        normalized = std::copysign(
                            (magnitude - binding.deadzone) /
                                (1.0F - binding.deadzone),
                            raw);
                    }
                    const float next_value = std::clamp(
                        normalized * binding.scale, -1.0F, 1.0F);
                    if (next_value != binding.value) {
                        binding.value = next_value;
                        binding.active = next_value != 0.0F;
                        changed = true;
                    }
                    handled = true;
                }
                break;

            case platform::InputEventType::GamepadDisconnected:
                if ((binding.kind == BindingKind::GamepadButton ||
                     binding.kind == BindingKind::GamepadAxis) &&
                    binding.device_id == event.device_id &&
                    (binding.active || binding.value != 0.0F)) {
                    binding.active = false;
                    binding.value = 0.0F;
                    changed = true;
                    handled = true;
                }
                break;

            case platform::InputEventType::TextInput:
            case platform::InputEventType::PointerMove:
            case platform::InputEventType::Scroll:
            case platform::InputEventType::GamepadConnected:
                break;
            }

            if (changed) changed_actions.push_back(binding.action_index);
        }

        if (handled) {
            std::sort(changed_actions.begin(), changed_actions.end());
            changed_actions.erase(
                std::unique(changed_actions.begin(), changed_actions.end()),
                changed_actions.end());
            for (const auto index : changed_actions)
                recompute_action(index, event.timestamp_ns);
        }
        return handled;
    }

    // Release every held input. Call this on application focus loss or when
    // the platform reports a global input reset, preventing stuck actions.
    void reset_all(std::uint64_t timestamp_ns = 0U) {
        std::lock_guard lock(mutex_);
        for (auto& binding : bindings_) {
            binding.active = false;
            binding.value = 0.0F;
        }
        for (std::size_t index = 0; index < actions_.size(); ++index)
            recompute_action(index, timestamp_ns);
    }

    [[nodiscard]] std::optional<ActionState> state(std::string_view action) const {
        std::lock_guard lock(mutex_);
        const auto found = action_indices_.find(std::string{action});
        if (found == action_indices_.end()) return std::nullopt;
        return actions_[found->second].state;
    }

    [[nodiscard]] std::vector<std::string> action_names() const {
        std::lock_guard lock(mutex_);
        std::vector<std::string> names;
        names.reserve(actions_.size());
        for (const auto& action : actions_) names.push_back(action.name);
        return names;
    }

    [[nodiscard]] std::size_t binding_count() const {
        std::lock_guard lock(mutex_);
        return bindings_.size();
    }

private:
    enum class BindingKind : std::uint8_t {
        Key,
        MouseButton,
        GamepadButton,
        GamepadAxis
    };

    struct Action final {
        std::string name;
        ActionState state;
    };

    struct Binding final {
        std::size_t action_index{0};
        BindingKind kind{BindingKind::Key};
        std::uint16_t control{0};
        platform::InputModifiers modifiers{};
        std::uint64_t device_id{0};
        float scale{1.0F};
        float deadzone{0.0F};
        bool active{false};
        float value{0.0F};
    };

    [[nodiscard]] ActionMapError add_binding(
        std::string_view action_name, Binding binding) {
        std::lock_guard lock(mutex_);
        const auto found = action_indices_.find(std::string{action_name});
        if (found == action_indices_.end()) return ActionMapError::UnknownAction;
        binding.action_index = found->second;
        bindings_.push_back(binding);
        return ActionMapError::None;
    }

    static bool valid_name(std::string_view name) noexcept {
        if (name.empty() || name.size() > 128U) return false;
        for (const unsigned char character : name) {
            const bool valid =
                (character >= 'a' && character <= 'z') ||
                (character >= 'A' && character <= 'Z') ||
                (character >= '0' && character <= '9') ||
                character == '_' || character == '-' ||
                character == '.' || character == '/';
            if (!valid) return false;
        }
        return true;
    }

    static bool valid_scale(float scale) noexcept {
        return std::isfinite(scale) && scale >= -1.0F &&
               scale <= 1.0F && scale != 0.0F;
    }

    static bool modifiers_equal(
        platform::InputModifiers left, platform::InputModifiers right) noexcept {
        return left.shift == right.shift &&
               left.control == right.control &&
               left.alt == right.alt &&
               left.super == right.super;
    }

    void recompute_action(std::size_t index, std::uint64_t timestamp_ns) noexcept {
        if (index >= actions_.size()) return;
        bool digital_down = false;
        float value = 0.0F;
        for (const auto& binding : bindings_) {
            if (binding.action_index != index) continue;
            if (binding.active &&
                (binding.kind == BindingKind::Key ||
                 binding.kind == BindingKind::MouseButton ||
                 binding.kind == BindingKind::GamepadButton)) {
                digital_down = true;
            }
            if (std::abs(binding.value) > std::abs(value))
                value = binding.value;
        }

        auto& state = actions_[index].state;
        const bool next_down = digital_down || std::abs(value) >= 0.5F;
        if (!state.down && next_down) state.just_pressed = true;
        if (state.down && !next_down) state.just_released = true;
        state.down = next_down;
        state.value = std::clamp(value, -1.0F, 1.0F);
        state.timestamp_ns = timestamp_ns;
    }

    mutable std::mutex mutex_;
    std::vector<Action> actions_;
    std::vector<Binding> bindings_;
    std::unordered_map<std::string, std::size_t> action_indices_;
};

} // namespace dream::input
