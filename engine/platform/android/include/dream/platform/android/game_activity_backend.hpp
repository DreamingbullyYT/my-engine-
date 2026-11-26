#pragma once

#if defined(__ANDROID__)

#include <dream/platform/application.hpp>
#include <dream/platform/input.hpp>
#include <dream/platform/surface.hpp>
#include <dream/platform/window.hpp>

#include <game-activity/native_app_glue/android_native_app_glue.h>

#include <cstdint>
#include <deque>
#include <string>
#include <thread>
#include <unordered_map>

namespace dream::platform {

// Adapter for an android_app supplied by the host application's android_main().
// It never defines android_main itself and never owns the Java/Kotlin Activity.
// All methods except monotonic_time_seconds() are intended for the GameActivity
// native-app-glue thread that constructed and initialized this adapter.
class AndroidGameActivityBackend final : public IPlatformBackend,
                                         public IWindowSystem,
                                         public ISurfaceSystem,
                                         public IInputSystem {
public:
    explicit AndroidGameActivityBackend(
        android_app* app, int poll_timeout_ms = 0) noexcept;
    ~AndroidGameActivityBackend() override;

    AndroidGameActivityBackend(const AndroidGameActivityBackend&) = delete;
    AndroidGameActivityBackend& operator=(const AndroidGameActivityBackend&) = delete;

    [[nodiscard]] bool initialize(
        const ApplicationConfig& config, std::string& error) override;
    [[nodiscard]] bool poll_events() override;
    [[nodiscard]] double monotonic_time_seconds() noexcept override;
    void shutdown() noexcept override;

    [[nodiscard]] bool create_window(
        const WindowConfig& config, WindowId& result, std::string& error) override;
    void destroy_window(WindowId window) noexcept override;
    [[nodiscard]] bool get_metrics(
        WindowId window, WindowMetrics& result) const noexcept override;
    [[nodiscard]] bool poll_window_event(WindowEvent& event) override;

    [[nodiscard]] bool acquire_surface(
        WindowId window, SurfaceId& result, NativeSurface& native,
        WindowMetrics& metrics, std::string& error) override;
    void release_surface(SurfaceId surface) noexcept override;
    [[nodiscard]] bool refresh_surface_metrics(
        SurfaceId surface, WindowMetrics& metrics) const noexcept override;

    [[nodiscard]] bool poll_input_event(InputEvent& event) override;

private:
    static void app_command_thunk(android_app* app, int32_t command);
    void handle_app_command(int32_t command) noexcept;
    void consume_input_buffer() noexcept;
    void consume_key_event(const GameActivityKeyEvent& event) noexcept;
    void consume_motion_event(const GameActivityMotionEvent& event) noexcept;
    void push_window_event(WindowEventType type) noexcept;
    void push_input_event(const InputEvent& event) noexcept;
    [[nodiscard]] bool on_owner_thread() const noexcept;
    [[nodiscard]] bool read_metrics(WindowMetrics& result) const noexcept;

    android_app* app_{nullptr};
    int poll_timeout_ms_{0};
    std::thread::id owner_thread_{};
    bool initialized_{false};
    bool window_created_{false};
    bool surface_available_{false};
    bool focused_{false};
    bool suspended_{false};
    WindowId window_id_{};
    std::uint64_t next_surface_id_{1};
    std::unordered_map<std::uint64_t, ANativeWindow*> surface_leases_;
    std::deque<WindowEvent> window_events_;
    std::deque<InputEvent> input_events_;

    void (*previous_app_command_)(android_app*, int32_t){nullptr};
    android_key_event_filter previous_key_filter_{nullptr};
    android_motion_event_filter previous_motion_filter_{nullptr};
};

} // namespace dream::platform

#endif // __ANDROID__
