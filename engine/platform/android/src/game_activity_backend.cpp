#include <dream/platform/android/game_activity_backend.hpp>

#if defined(__ANDROID__)

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <limits>
#include <mutex>
#include <unordered_map>

#include <android/configuration.h>
#include <android/input.h>
#include <android/keycodes.h>
#include <android/looper.h>
#include <android/native_window.h>

namespace dream::platform {
namespace {

constexpr std::size_t kMaxQueuedEvents = 4096;
constexpr std::size_t kMaxImeTextBytes = 65536U;
constexpr std::uint64_t kMaxSurfaceId = std::numeric_limits<std::uint64_t>::max();

std::mutex g_backend_registry_mutex;
std::unordered_map<android_app*, AndroidGameActivityBackend*> g_backend_registry;

[[nodiscard]] std::uint64_t positive_device_id(std::int32_t value) noexcept {
    return value < 0 ? 0U : static_cast<std::uint64_t>(value);
}

[[nodiscard]] std::uint16_t usb_hid_usage(std::int32_t key_code) noexcept {
    if (key_code >= AKEYCODE_A && key_code <= AKEYCODE_Z) {
        return static_cast<std::uint16_t>(0x04 + key_code - AKEYCODE_A);
    }
    if (key_code >= AKEYCODE_1 && key_code <= AKEYCODE_9) {
        return static_cast<std::uint16_t>(0x1E + key_code - AKEYCODE_1);
    }
    if (key_code == AKEYCODE_0) return 0x27;

    switch (key_code) {
    case AKEYCODE_ENTER: return 0x28;
    case AKEYCODE_ESCAPE:
    case AKEYCODE_BACK: return 0x29;
    case AKEYCODE_DEL: return 0x2A;
    case AKEYCODE_TAB: return 0x2B;
    case AKEYCODE_SPACE: return 0x2C;
    case AKEYCODE_MINUS: return 0x2D;
    case AKEYCODE_EQUALS: return 0x2E;
    case AKEYCODE_LEFT_BRACKET: return 0x2F;
    case AKEYCODE_RIGHT_BRACKET: return 0x30;
    case AKEYCODE_BACKSLASH: return 0x31;
    case AKEYCODE_SEMICOLON: return 0x33;
    case AKEYCODE_APOSTROPHE: return 0x34;
    case AKEYCODE_GRAVE: return 0x35;
    case AKEYCODE_COMMA: return 0x36;
    case AKEYCODE_PERIOD: return 0x37;
    case AKEYCODE_SLASH: return 0x38;
    case AKEYCODE_CAPS_LOCK: return 0x39;
    case AKEYCODE_F1: return 0x3A;
    case AKEYCODE_F2: return 0x3B;
    case AKEYCODE_F3: return 0x3C;
    case AKEYCODE_F4: return 0x3D;
    case AKEYCODE_F5: return 0x3E;
    case AKEYCODE_F6: return 0x3F;
    case AKEYCODE_F7: return 0x40;
    case AKEYCODE_F8: return 0x41;
    case AKEYCODE_F9: return 0x42;
    case AKEYCODE_F10: return 0x43;
    case AKEYCODE_F11: return 0x44;
    case AKEYCODE_F12: return 0x45;
    case AKEYCODE_SYSRQ: return 0x46;
    case AKEYCODE_SCROLL_LOCK: return 0x47;
    case AKEYCODE_BREAK: return 0x48;
    case AKEYCODE_INSERT: return 0x49;
    case AKEYCODE_MOVE_HOME: return 0x4A;
    case AKEYCODE_PAGE_UP: return 0x4B;
    case AKEYCODE_FORWARD_DEL: return 0x4C;
    case AKEYCODE_MOVE_END: return 0x4D;
    case AKEYCODE_PAGE_DOWN: return 0x4E;
    case AKEYCODE_DPAD_RIGHT: return 0x4F;
    case AKEYCODE_DPAD_LEFT: return 0x50;
    case AKEYCODE_DPAD_DOWN: return 0x51;
    case AKEYCODE_DPAD_UP: return 0x52;
    case AKEYCODE_NUM_LOCK: return 0x53;
    case AKEYCODE_NUMPAD_DIVIDE: return 0x54;
    case AKEYCODE_NUMPAD_MULTIPLY: return 0x55;
    case AKEYCODE_NUMPAD_SUBTRACT: return 0x56;
    case AKEYCODE_NUMPAD_ADD: return 0x57;
    case AKEYCODE_NUMPAD_ENTER: return 0x58;
    case AKEYCODE_NUMPAD_1: return 0x59;
    case AKEYCODE_NUMPAD_2: return 0x5A;
    case AKEYCODE_NUMPAD_3: return 0x5B;
    case AKEYCODE_NUMPAD_4: return 0x5C;
    case AKEYCODE_NUMPAD_5: return 0x5D;
    case AKEYCODE_NUMPAD_6: return 0x5E;
    case AKEYCODE_NUMPAD_7: return 0x5F;
    case AKEYCODE_NUMPAD_8: return 0x60;
    case AKEYCODE_NUMPAD_9: return 0x61;
    case AKEYCODE_NUMPAD_0: return 0x62;
    case AKEYCODE_NUMPAD_DOT: return 0x63;
    case AKEYCODE_CTRL_LEFT: return 0xE0;
    case AKEYCODE_SHIFT_LEFT: return 0xE1;
    case AKEYCODE_ALT_LEFT: return 0xE2;
    case AKEYCODE_META_LEFT: return 0xE3;
    case AKEYCODE_CTRL_RIGHT: return 0xE4;
    case AKEYCODE_SHIFT_RIGHT: return 0xE5;
    case AKEYCODE_ALT_RIGHT: return 0xE6;
    case AKEYCODE_META_RIGHT: return 0xE7;
    default: return 0;
    }
}

[[nodiscard]] InputModifiers modifiers_from_meta_state(std::int32_t state) noexcept {
    return {
        (state & AMETA_SHIFT_ON) != 0,
        (state & AMETA_CTRL_ON) != 0,
        (state & AMETA_ALT_ON) != 0,
        (state & AMETA_META_ON) != 0
    };
}

[[nodiscard]] PointerKind pointer_kind(
    std::int32_t source, std::int32_t tool_type) noexcept {
    if ((source & AINPUT_SOURCE_MOUSE) == AINPUT_SOURCE_MOUSE ||
        tool_type == AMOTION_EVENT_TOOL_TYPE_MOUSE) {
        return PointerKind::Mouse;
    }
    if (tool_type == AMOTION_EVENT_TOOL_TYPE_STYLUS ||
        tool_type == AMOTION_EVENT_TOOL_TYPE_ERASER) {
        return PointerKind::Pen;
    }
    if (tool_type == AMOTION_EVENT_TOOL_TYPE_FINGER) return PointerKind::Touch;
    return PointerKind::Unknown;
}

[[nodiscard]] std::uint8_t mouse_button(std::int32_t button_state) noexcept {
    if ((button_state & AMOTION_EVENT_BUTTON_PRIMARY) != 0) return 1;
    if ((button_state & AMOTION_EVENT_BUTTON_SECONDARY) != 0) return 2;
    if ((button_state & AMOTION_EVENT_BUTTON_TERTIARY) != 0) return 3;
    if ((button_state & AMOTION_EVENT_BUTTON_BACK) != 0) return 4;
    if ((button_state & AMOTION_EVENT_BUTTON_FORWARD) != 0) return 5;
    return 0;
}

[[nodiscard]] float bounded_axis(float value) noexcept {
    if (!std::isfinite(value)) return 0.0F;
    return std::clamp(value, -1.0F, 1.0F);
}

} // namespace

AndroidGameActivityBackend::AndroidGameActivityBackend(
    android_app* app, int poll_timeout_ms) noexcept
    : app_(app),
      poll_timeout_ms_(std::max(0, poll_timeout_ms)) {}

AndroidGameActivityBackend::~AndroidGameActivityBackend() {
    shutdown();
}

bool AndroidGameActivityBackend::initialize(
    const ApplicationConfig& config, std::string& error) {
    if (initialized_) {
        error = "GameActivity backend is already initialized.";
        return false;
    }
    if (app_ == nullptr || app_->looper == nullptr) {
        error = "GameActivity backend requires a valid android_app and ALooper.";
        return false;
    }
    if (config.name.empty() ||
        !std::isfinite(config.max_frame_delta_seconds) ||
        config.max_frame_delta_seconds <= 0.0) {
        error = "Application configuration is invalid.";
        return false;
    }

    owner_thread_ = std::this_thread::get_id();
    {
        std::lock_guard lock(g_backend_registry_mutex);
        if (g_backend_registry.contains(app_)) {
            error = "An Android GameActivity backend is already attached to this android_app.";
            return false;
        }
        g_backend_registry.emplace(app_, this);
    }

    previous_app_command_ = app_->onAppCmd;
    previous_key_filter_ = app_->keyEventFilter;
    previous_motion_filter_ = app_->motionEventFilter;
    app_->onAppCmd = &AndroidGameActivityBackend::app_command_thunk;

    // The glue's default motion filter is touchscreen-only. Clearing the
    // filters allows mouse/stylus/gamepad sources to reach the input buffer.
    android_app_set_key_event_filter(app_, nullptr);
    android_app_set_motion_event_filter(app_, nullptr);
    constexpr std::array<std::int32_t, 10> gamepad_axes{
        AMOTION_EVENT_AXIS_Z, AMOTION_EVENT_AXIS_RZ,
        AMOTION_EVENT_AXIS_RX, AMOTION_EVENT_AXIS_RY,
        AMOTION_EVENT_AXIS_HAT_X, AMOTION_EVENT_AXIS_HAT_Y,
        AMOTION_EVENT_AXIS_LTRIGGER, AMOTION_EVENT_AXIS_RTRIGGER,
        AMOTION_EVENT_AXIS_BRAKE, AMOTION_EVENT_AXIS_GAS
    };
    for (const auto axis : gamepad_axes) GameActivityPointerAxes_enableAxis(axis);

    initialized_ = true;
    surface_available_ = app_->window != nullptr;
    focused_ = false;
    suspended_ = false;
    error.clear();
    return true;
}

bool AndroidGameActivityBackend::poll_events() {
    if (!initialized_ || !on_owner_thread() || app_ == nullptr) return false;

    int events = 0;
    void* data = nullptr;
    const int result = ALooper_pollOnce(
        poll_timeout_ms_, nullptr, &events, &data);
    if (result == LOOPER_ID_MAIN && data != nullptr) {
        auto* source = static_cast<android_poll_source*>(data);
        if (source->process != nullptr) source->process(app_, source);
    }

    consume_input_buffer();
    consume_text_input_state();
    return app_->destroyRequested == 0;
}

double AndroidGameActivityBackend::monotonic_time_seconds() noexcept {
    const auto now = std::chrono::steady_clock::now().time_since_epoch();
    return std::chrono::duration<double>(now).count();
}

void AndroidGameActivityBackend::shutdown() noexcept {
    if (!initialized_) return;

    {
        std::lock_guard lock(g_backend_registry_mutex);
        const auto entry = g_backend_registry.find(app_);
        if (entry != g_backend_registry.end() && entry->second == this) {
            g_backend_registry.erase(entry);
        }
    }

    if (app_ != nullptr) {
        if (app_->onAppCmd == &AndroidGameActivityBackend::app_command_thunk) {
            app_->onAppCmd = previous_app_command_;
        }
        android_app_set_key_event_filter(app_, previous_key_filter_);
        android_app_set_motion_event_filter(app_, previous_motion_filter_);
    }

    for (auto& [id, native_window] : surface_leases_) {
        (void)id;
        if (native_window != nullptr) ANativeWindow_release(native_window);
    }
    surface_leases_.clear();
    window_events_.clear();
    input_events_.clear();
    surface_available_ = false;
    window_created_ = false;
    focused_ = false;
    suspended_ = true;
    initialized_ = false;
    app_ = nullptr;
}

bool AndroidGameActivityBackend::create_window(
    const WindowConfig& config, WindowId& result, std::string& error) {
    if (!initialized_ || !on_owner_thread() || app_ == nullptr) {
        error = "GameActivity backend is not initialized on its owner thread.";
        return false;
    }
    if (window_created_) {
        error = "GameActivity backend exposes one host-owned SurfaceView window.";
        return false;
    }
    if (config.width == 0 || config.height == 0 || config.title.empty()) {
        error = "Window configuration requires a title and non-zero extent.";
        return false;
    }
    window_created_ = true;
    window_id_ = WindowId{1};
    result = window_id_;
    error.clear();
    if (surface_available_) {
        push_window_event(WindowEventType::SurfaceCreated);
        push_window_event(WindowEventType::Resized);
    }
    return true;
}

void AndroidGameActivityBackend::destroy_window(WindowId window) noexcept {
    if (!on_owner_thread() || !window_created_ || window != window_id_) return;
    window_created_ = false;
    push_window_event(WindowEventType::CloseRequested);
    window_id_ = {};
}

bool AndroidGameActivityBackend::get_metrics(
    WindowId window, WindowMetrics& result) const noexcept {
    return window_created_ && window == window_id_ && read_metrics(result);
}

bool AndroidGameActivityBackend::poll_window_event(WindowEvent& event) {
    if (!on_owner_thread() || window_events_.empty()) return false;
    event = window_events_.front();
    window_events_.pop_front();
    return true;
}

bool AndroidGameActivityBackend::acquire_surface(
    WindowId window, SurfaceId& result, NativeSurface& native,
    WindowMetrics& metrics, std::string& error) {
    if (!initialized_ || !on_owner_thread() || !window_created_ ||
        window != window_id_ || !surface_available_ || app_ == nullptr ||
        app_->window == nullptr) {
        error = "GameActivity has no available native window surface.";
        return false;
    }
    if (next_surface_id_ == 0 || next_surface_id_ == kMaxSurfaceId) {
        error = "Android surface lease ID space is exhausted.";
        return false;
    }
    if (!read_metrics(metrics)) {
        error = "Android native window metrics are unavailable.";
        return false;
    }

    ANativeWindow_acquire(app_->window);
    const auto id = next_surface_id_++;
    try {
        surface_leases_.emplace(id, app_->window);
    } catch (...) {
        ANativeWindow_release(app_->window);
        error = "Unable to allocate an Android surface lease.";
        return false;
    }
    result = SurfaceId{id};
    native = NativeSurface{NativeSurfaceKind::Android, nullptr, app_->window};
    error.clear();
    return true;
}

void AndroidGameActivityBackend::release_surface(SurfaceId surface) noexcept {
    if (!on_owner_thread() || !surface) return;
    const auto entry = surface_leases_.find(surface.value);
    if (entry == surface_leases_.end()) return;
    if (entry->second != nullptr) ANativeWindow_release(entry->second);
    surface_leases_.erase(entry);
}

bool AndroidGameActivityBackend::refresh_surface_metrics(
    SurfaceId surface, WindowMetrics& metrics) const noexcept {
    if (!on_owner_thread() || !surface_available_ || app_ == nullptr ||
        app_->window == nullptr) {
        return false;
    }
    const auto entry = surface_leases_.find(surface.value);
    return entry != surface_leases_.end() &&
        entry->second == app_->window && read_metrics(metrics);
}

bool AndroidGameActivityBackend::poll_input_event(InputEvent& event) {
    if (!on_owner_thread() || input_events_.empty()) return false;
    event = input_events_.front();
    input_events_.pop_front();
    return true;
}

void AndroidGameActivityBackend::app_command_thunk(
    android_app* app, int32_t command) {
    AndroidGameActivityBackend* backend = nullptr;
    {
        std::lock_guard lock(g_backend_registry_mutex);
        const auto entry = g_backend_registry.find(app);
        if (entry != g_backend_registry.end()) backend = entry->second;
    }
    if (backend == nullptr) return;

    if (backend->previous_app_command_ != nullptr &&
        backend->previous_app_command_ != &AndroidGameActivityBackend::app_command_thunk) {
        backend->previous_app_command_(app, command);
    }
    backend->handle_app_command(command);
}

void AndroidGameActivityBackend::handle_app_command(int32_t command) noexcept {
    switch (command) {
    case APP_CMD_INIT_WINDOW:
        surface_available_ = app_ != nullptr && app_->window != nullptr;
        if (surface_available_ && window_created_) {
            push_window_event(WindowEventType::SurfaceCreated);
            push_window_event(WindowEventType::Resized);
        }
        break;
    case APP_CMD_TERM_WINDOW:
        surface_available_ = false;
        if (window_created_) push_window_event(WindowEventType::SurfaceDestroyed);
        break;
    case APP_CMD_WINDOW_RESIZED:
    case APP_CMD_WINDOW_REDRAW_NEEDED:
    case APP_CMD_CONFIG_CHANGED:
    case APP_CMD_CONTENT_RECT_CHANGED:
        push_window_event(WindowEventType::Resized);
        break;
    case APP_CMD_GAINED_FOCUS:
        focused_ = true;
        push_window_event(WindowEventType::FocusGained);
        break;
    case APP_CMD_LOST_FOCUS:
        focused_ = false;
        push_window_event(WindowEventType::FocusLost);
        break;
    case APP_CMD_PAUSE:
    case APP_CMD_STOP:
        if (!suspended_) {
            suspended_ = true;
            push_window_event(WindowEventType::Suspended);
        }
        break;
    case APP_CMD_RESUME:
        if (suspended_) {
            suspended_ = false;
            push_window_event(WindowEventType::Resumed);
        }
        break;
    case APP_CMD_LOW_MEMORY:
        push_window_event(WindowEventType::MemoryLow);
        break;
    case APP_CMD_DESTROY:
        push_window_event(WindowEventType::CloseRequested);
        break;
    default:
        break;
    }
}

void AndroidGameActivityBackend::consume_input_buffer() noexcept {
    if (app_ == nullptr) return;
    android_input_buffer* buffer = android_app_swap_input_buffers(app_);
    if (buffer == nullptr) return;

    if (buffer->keyEvents != nullptr) {
        for (std::uint64_t index = 0; index < buffer->keyEventsCount; ++index) {
            consume_key_event(buffer->keyEvents[index]);
        }
    }
    if (buffer->motionEvents != nullptr) {
        for (std::uint64_t index = 0; index < buffer->motionEventsCount; ++index) {
            consume_motion_event(buffer->motionEvents[index]);
        }
    }
    android_app_clear_motion_events(buffer);
    android_app_clear_key_events(buffer);
}

void AndroidGameActivityBackend::consume_text_input_state() noexcept {
    if (app_ == nullptr || app_->activity == nullptr ||
        app_->textInputState == 0) return;
    GameActivity_getTextInputState(
        app_->activity,
        &AndroidGameActivityBackend::text_input_state_callback,
        this);
}

void AndroidGameActivityBackend::text_input_state_callback(
    void* context, const GameTextInputState* state) noexcept {
    auto* backend = static_cast<AndroidGameActivityBackend*>(context);
    if (backend == nullptr || backend->app_ == nullptr) return;

    if (state != nullptr && state->text_UTF8 != nullptr &&
        state->text_length >= 0 &&
        static_cast<std::size_t>(state->text_length) <= kMaxImeTextBytes) {
        try {
            InputEvent event{};
            event.type = InputEventType::TextInput;
            event.timestamp_ns = static_cast<std::uint64_t>(
                std::chrono::duration_cast<std::chrono::nanoseconds>(
                    std::chrono::steady_clock::now().time_since_epoch()).count());
            event.text_utf8.assign(
                state->text_UTF8, static_cast<std::size_t>(state->text_length));
            event.has_text_snapshot = true;
            event.selection_start = state->selection.start;
            event.selection_end = state->selection.end;
            event.composing_start = state->composingRegion.start;
            event.composing_end = state->composingRegion.end;
            backend->push_input_event(event);
        } catch (...) {
            // Text snapshots are optional under memory pressure; keep the loop alive.
        }
    }

    // GameActivity exposes a dirty flag, not a counted queue. Clear it after
    // copying the state while it is valid inside this callback.
    backend->app_->textInputState = 0;
}

bool AndroidGameActivityBackend::show_soft_keyboard(
    std::string_view text) noexcept {
    if (!initialized_ || !on_owner_thread() || app_ == nullptr ||
        app_->activity == nullptr || text.size() > kMaxImeTextBytes ||
        text.size() > static_cast<std::size_t>(
            std::numeric_limits<std::int32_t>::max())) {
        return false;
    }

    std::string owned_text;
    try {
        owned_text.assign(text);
    } catch (...) {
        return false;
    }
    GameTextInputState state{};
    state.text_UTF8 = owned_text.c_str();
    state.text_length = static_cast<std::int32_t>(owned_text.size());
    const auto cursor = state.text_length;
    state.selection = GameTextInputSpan{cursor, cursor};
    state.composingRegion = GameTextInputSpan{SPAN_UNDEFINED, SPAN_UNDEFINED};
    GameActivity_setTextInputState(app_->activity, &state);
    GameActivity_showSoftInput(app_->activity, GAMEACTIVITY_SHOW_SOFT_INPUT_IMPLICIT);
    return true;
}

bool AndroidGameActivityBackend::hide_soft_keyboard() noexcept {
    if (!initialized_ || app_ == nullptr || app_->activity == nullptr)
        return false;
    GameActivity_hideSoftInput(app_->activity, 0U);
    return true;
}

void AndroidGameActivityBackend::consume_key_event(
    const GameActivityKeyEvent& source) noexcept {
    const auto device_id = positive_device_id(source.deviceId);
    const bool gamepad =
        (source.source & AINPUT_SOURCE_GAMEPAD) == AINPUT_SOURCE_GAMEPAD ||
        (source.source & AINPUT_SOURCE_DPAD) == AINPUT_SOURCE_DPAD;

    if (source.action != AKEY_EVENT_ACTION_DOWN &&
        source.action != AKEY_EVENT_ACTION_UP) {
        return;
    }

    InputEvent event{};
    event.timestamp_ns = source.eventTime < 0 ? 0U :
        static_cast<std::uint64_t>(source.eventTime);
    event.device_id = device_id;
    event.modifiers = modifiers_from_meta_state(source.metaState);

    if (gamepad) {
        event.type = source.action == AKEY_EVENT_ACTION_DOWN
            ? InputEventType::GamepadButtonDown
            : InputEventType::GamepadButtonUp;
        event.gamepad_control = source.keyCode < 0 ? 0U :
            static_cast<std::uint16_t>(std::min(source.keyCode, 65535));
        push_input_event(event);
        return;
    }

    event.key_code = usb_hid_usage(source.keyCode);
    if (event.key_code != 0) {
        event.type = source.action == AKEY_EVENT_ACTION_DOWN
            ? InputEventType::KeyDown : InputEventType::KeyUp;
        push_input_event(event);
    }
    if (source.action == AKEY_EVENT_ACTION_DOWN &&
        source.unicodeChar > 0 &&
        static_cast<std::uint32_t>(source.unicodeChar) <= 0x10FFFFU) {
        event = {};
        event.type = InputEventType::TextInput;
        event.timestamp_ns = source.eventTime < 0 ? 0U :
            static_cast<std::uint64_t>(source.eventTime);
        event.device_id = device_id;
        event.codepoint = static_cast<char32_t>(source.unicodeChar);
        event.modifiers = modifiers_from_meta_state(source.metaState);
        push_input_event(event);
    }
}

void AndroidGameActivityBackend::consume_motion_event(
    const GameActivityMotionEvent& source) noexcept {
    const auto device_id = positive_device_id(source.deviceId);
    const std::int32_t action = source.action & AMOTION_EVENT_ACTION_MASK;
    const std::uint64_t timestamp = source.eventTime < 0 ? 0U :
        static_cast<std::uint64_t>(source.eventTime);
    const bool joystick =
        (source.source & AINPUT_SOURCE_CLASS_MASK) ==
        AINPUT_SOURCE_CLASS_JOYSTICK;

    if (joystick && source.pointerCount > 0) {
        const auto& axes = source.pointers[0];
        constexpr std::array<std::int32_t, 12> axis_ids{
            AMOTION_EVENT_AXIS_X, AMOTION_EVENT_AXIS_Y,
            AMOTION_EVENT_AXIS_Z, AMOTION_EVENT_AXIS_RZ,
            AMOTION_EVENT_AXIS_RX, AMOTION_EVENT_AXIS_RY,
            AMOTION_EVENT_AXIS_HAT_X, AMOTION_EVENT_AXIS_HAT_Y,
            AMOTION_EVENT_AXIS_LTRIGGER, AMOTION_EVENT_AXIS_RTRIGGER,
            AMOTION_EVENT_AXIS_BRAKE, AMOTION_EVENT_AXIS_GAS
        };
        for (const auto axis_id : axis_ids) {
            const float value = GameActivityPointerAxes_getAxisValue(&axes, axis_id);
            InputEvent event{};
            event.type = InputEventType::GamepadAxis;
            event.timestamp_ns = timestamp;
            event.device_id = device_id;
            event.gamepad_control = static_cast<std::uint16_t>(axis_id);
            event.axis_value = bounded_axis(value);
            push_input_event(event);
        }
        return;
    }

    const auto pointer_count = std::min<std::uint32_t>(
        source.pointerCount, GAMEACTIVITY_MAX_NUM_POINTERS_IN_MOTION_EVENT);
    if (pointer_count == 0) return;

    const auto emit_pointer = [&](std::uint32_t index, InputEventType type) {
        if (index >= pointer_count) return;
        const auto& pointer = source.pointers[index];
        InputEvent event{};
        event.type = type;
        event.timestamp_ns = timestamp;
        event.device_id = device_id;
        event.pointer_id = pointer.id < 0 ? 0U :
            static_cast<std::uint64_t>(pointer.id);
        event.pointer_kind = pointer_kind(source.source, pointer.toolType);
        WindowMetrics metrics{};
        const float scale = read_metrics(metrics) ? metrics.scale_x : 1.0F;
        event.x = GameActivityPointerAxes_getX(&pointer) / scale;
        event.y = GameActivityPointerAxes_getY(&pointer) / scale;
        event.button = mouse_button(source.buttonState);
        event.modifiers = modifiers_from_meta_state(source.metaState);
        push_input_event(event);
    };

    const auto action_index = static_cast<std::uint32_t>(
        (source.action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >>
        AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT);

    switch (action) {
    case AMOTION_EVENT_ACTION_DOWN:
    case AMOTION_EVENT_ACTION_POINTER_DOWN:
        emit_pointer(action == AMOTION_EVENT_ACTION_DOWN ? 0U : action_index,
                     InputEventType::PointerDown);
        break;
    case AMOTION_EVENT_ACTION_UP:
    case AMOTION_EVENT_ACTION_POINTER_UP:
        emit_pointer(action == AMOTION_EVENT_ACTION_UP ? 0U : action_index,
                     InputEventType::PointerUp);
        break;
    case AMOTION_EVENT_ACTION_CANCEL:
        for (std::uint32_t i = 0; i < pointer_count; ++i)
            emit_pointer(i, InputEventType::PointerCancel);
        break;
    case AMOTION_EVENT_ACTION_MOVE:
    case AMOTION_EVENT_ACTION_HOVER_MOVE:
        for (std::uint32_t i = 0; i < pointer_count; ++i)
            emit_pointer(i, InputEventType::PointerMove);
        break;
    case AMOTION_EVENT_ACTION_SCROLL: {
        const auto& pointer = source.pointers[0];
        InputEvent event{};
        event.type = InputEventType::Scroll;
        event.timestamp_ns = timestamp;
        event.device_id = device_id;
        event.pointer_id = pointer.id < 0 ? 0U :
            static_cast<std::uint64_t>(pointer.id);
        event.pointer_kind = pointer_kind(source.source, pointer.toolType);
        WindowMetrics metrics{};
        const float scale = read_metrics(metrics) ? metrics.scale_x : 1.0F;
        event.x = GameActivityPointerAxes_getX(&pointer) / scale;
        event.y = GameActivityPointerAxes_getY(&pointer) / scale;
        event.delta_x = GameActivityPointerAxes_getAxisValue(
            &pointer, AMOTION_EVENT_AXIS_HSCROLL);
        event.delta_y = GameActivityPointerAxes_getAxisValue(
            &pointer, AMOTION_EVENT_AXIS_VSCROLL);
        event.modifiers = modifiers_from_meta_state(source.metaState);
        push_input_event(event);
        break;
    }
    case AMOTION_EVENT_ACTION_BUTTON_PRESS:
    case AMOTION_EVENT_ACTION_BUTTON_RELEASE:
        emit_pointer(0U, action == AMOTION_EVENT_ACTION_BUTTON_PRESS
            ? InputEventType::PointerDown : InputEventType::PointerUp);
        break;
    default:
        break;
    }
}

void AndroidGameActivityBackend::push_window_event(WindowEventType type) noexcept {
    try {
        if (window_events_.size() == kMaxQueuedEvents) window_events_.pop_front();
        window_events_.push_back(WindowEvent{type, window_id_, {}});
        if (type == WindowEventType::Resized ||
            type == WindowEventType::SurfaceCreated) {
            (void)read_metrics(window_events_.back().metrics);
        }
    } catch (...) {
        // Platform callbacks must not propagate allocation failures into glue.
    }
}

void AndroidGameActivityBackend::push_input_event(const InputEvent& event) noexcept {
    try {
        if (input_events_.size() == kMaxQueuedEvents) input_events_.pop_front();
        input_events_.push_back(event);
    } catch (...) {
        // Input queue exhaustion drops an event rather than destabilizing the app thread.
    }
}

bool AndroidGameActivityBackend::on_owner_thread() const noexcept {
    return owner_thread_ == std::this_thread::get_id();
}

bool AndroidGameActivityBackend::read_metrics(WindowMetrics& result) const noexcept {
    if (app_ == nullptr || app_->window == nullptr || !surface_available_) return false;
    const int width = ANativeWindow_getWidth(app_->window);
    const int height = ANativeWindow_getHeight(app_->window);
    if (width <= 0 || height <= 0) return false;

    int density = app_->config != nullptr
        ? AConfiguration_getDensity(app_->config)
        : ACONFIGURATION_DENSITY_DEFAULT;
    if (density <= 0 || density == ACONFIGURATION_DENSITY_ANY) {
        density = ACONFIGURATION_DENSITY_DEFAULT;
    }
    const float scale = static_cast<float>(density) /
        static_cast<float>(ACONFIGURATION_DENSITY_DEFAULT);
    if (!std::isfinite(scale) || scale <= 0.0F) return false;

    const auto pixel_width = static_cast<std::uint32_t>(width);
    const auto pixel_height = static_cast<std::uint32_t>(height);
    result.pixel_width = pixel_width;
    result.pixel_height = pixel_height;
    result.scale_x = scale;
    result.scale_y = scale;
    result.logical_width = static_cast<std::uint32_t>(
        std::lround(static_cast<double>(pixel_width) / scale));
    result.logical_height = static_cast<std::uint32_t>(
        std::lround(static_cast<double>(pixel_height) / scale));
    result.focused = focused_;
    result.minimized = suspended_;
    return true;
}

} // namespace dream::platform

#endif // __ANDROID__
