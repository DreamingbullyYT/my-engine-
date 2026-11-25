#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace dream::platform {

struct WindowId final {
    std::uint64_t value{0};

    [[nodiscard]] constexpr explicit operator bool() const noexcept {
        return value != 0;
    }

    friend constexpr bool operator==(WindowId, WindowId) noexcept = default;
};

enum class WindowMode : std::uint8_t {
    Windowed,
    BorderlessFullscreen,
    ExclusiveFullscreen
};

struct WindowConfig final {
    std::string title{"Dream Engine"};
    std::uint32_t width{1280};
    std::uint32_t height{720};
    WindowMode mode{WindowMode::Windowed};
    bool resizable{true};
    bool high_dpi{true};
};

struct WindowMetrics final {
    std::uint32_t logical_width{0};
    std::uint32_t logical_height{0};
    std::uint32_t pixel_width{0};
    std::uint32_t pixel_height{0};
    float scale_x{1.0F};
    float scale_y{1.0F};
    bool focused{false};
    bool minimized{false};
};

enum class WindowEventType : std::uint8_t {
    CloseRequested,
    Resized,
    FocusGained,
    FocusLost,
    Minimized,
    Restored
};

struct WindowEvent final {
    WindowEventType type{WindowEventType::CloseRequested};
    WindowId window{};
    WindowMetrics metrics{};
};

class IWindowSystem {
public:
    virtual ~IWindowSystem() = default;

    [[nodiscard]] virtual bool create_window(
        const WindowConfig& config, WindowId& result, std::string& error) = 0;
    virtual void destroy_window(WindowId window) noexcept = 0;
    [[nodiscard]] virtual bool get_metrics(
        WindowId window, WindowMetrics& result) const noexcept = 0;
    [[nodiscard]] virtual bool poll_window_event(WindowEvent& event) = 0;
};

} // namespace dream::platform
