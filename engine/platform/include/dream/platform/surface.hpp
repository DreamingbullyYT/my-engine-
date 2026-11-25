#pragma once

#include <cstdint>
#include <string>

#include <dream/platform/window.hpp>

namespace dream::platform {

enum class NativeSurfaceKind : std::uint8_t {
    Unknown,
    Win32,
    X11,
    Wayland,
    Android,
    Apple,
    Headless
};

struct SurfaceId final {
    std::uint64_t value{0};

    [[nodiscard]] constexpr explicit operator bool() const noexcept {
        return value != 0;
    }

    friend constexpr bool operator==(SurfaceId, SurfaceId) noexcept = default;
};

struct NativeSurface final {
    NativeSurfaceKind kind{NativeSurfaceKind::Unknown};
    void* display{nullptr};
    void* window{nullptr};
};

// Native pointers are borrowed. They remain valid only until release_surface()
// or the associated platform surface is destroyed.
class ISurfaceSystem {
public:
    virtual ~ISurfaceSystem() = default;

    [[nodiscard]] virtual bool acquire_surface(
        WindowId window, SurfaceId& result, NativeSurface& native,
        WindowMetrics& metrics, std::string& error) = 0;
    virtual void release_surface(SurfaceId surface) noexcept = 0;
    [[nodiscard]] virtual bool refresh_surface_metrics(
        SurfaceId surface, WindowMetrics& metrics) const noexcept = 0;
};

} // namespace dream::platform
