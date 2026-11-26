#pragma once

#include <dream/platform/surface.hpp>
#include <dream/render/render_types.hpp>

#include <string>

namespace dream::render {

enum class RenderError : std::uint8_t {
    None,
    AlreadyInitialized,
    NotInitialized,
    InvalidWindow,
    SurfaceAcquisitionFailed,
    SurfaceUnavailable,
    SurfaceSuspended,
    BackendInitializationFailed,
    BackendResizeFailed,
    InvalidFrame,
    BackendRenderFailed
};

struct RenderResult final {
    RenderError error{RenderError::None};
    FrameValidationError frame_error{FrameValidationError::None};
    std::string message;

    [[nodiscard]] explicit operator bool() const noexcept {
        return error == RenderError::None;
    }
};

// A backend owns its GPU/device objects, not the native surface. Native surface
// pointers are borrowed and remain valid only while the platform surface is
// acquired. shutdown() must stop all backend use before RenderDevice releases it.
//
// render() receives borrowed spans in RenderFramePacket. It must not retain
// pointers or spans after the call returns. Backend implementations must accept
// both 2D and 3D in the same frame when both capabilities are advertised.
class IRenderBackend {
public:
    virtual ~IRenderBackend() = default;

    [[nodiscard]] virtual bool initialize(
        platform::NativeSurface native_surface,
        const RenderSurfaceInfo& surface,
        RenderCapabilities& capabilities,
        std::string& error) = 0;

    [[nodiscard]] virtual bool resize(
        const RenderSurfaceInfo& surface, std::string& error) = 0;

    [[nodiscard]] virtual bool render(
        const RenderFramePacket& frame, std::string& error) = 0;

    virtual void shutdown() noexcept = 0;
};

// Owns the platform surface lease and sequences its lifetime around the
// backend. It is intentionally single-threaded; call it from the render thread.
class RenderDevice final {
public:
    RenderDevice(platform::ISurfaceSystem& surfaces, IRenderBackend& backend) noexcept
        : surfaces_(surfaces), backend_(backend) {}

    RenderDevice(const RenderDevice&) = delete;
    RenderDevice& operator=(const RenderDevice&) = delete;

    ~RenderDevice() { shutdown(); }

    [[nodiscard]] bool initialized() const noexcept { return initialized_; }

    [[nodiscard]] const RenderCapabilities& capabilities() const noexcept {
        return capabilities_;
    }

    [[nodiscard]] const RenderSurfaceInfo& surface_info() const noexcept {
        return surface_info_;
    }

    [[nodiscard]] RenderResult initialize(platform::WindowId window) {
        if (initialized_)
            return {RenderError::AlreadyInitialized, {}, "RenderDevice is already initialized"};
        if (!window)
            return {RenderError::InvalidWindow, {}, "WindowId is invalid"};

        platform::NativeSurface native{};
        platform::WindowMetrics metrics{};
        std::string error;
        if (!surfaces_.acquire_surface(
                window, surface_id_, native, metrics, error)) {
            reset_surface_lease();
            return {
                RenderError::SurfaceAcquisitionFailed, {}, std::move(error)
            };
        }

        if (!surface_id_ || metrics.minimized ||
            metrics.pixel_width == 0U || metrics.pixel_height == 0U) {
            reset_surface_lease();
            return {
                RenderError::SurfaceUnavailable, {},
                "Native surface has no drawable pixel extent"
            };
        }

        surface_info_ = make_surface_info(metrics);
        if (!backend_.initialize(native, surface_info_, capabilities_, error)) {
            backend_.shutdown();
            reset_surface_lease();
            return {
                RenderError::BackendInitializationFailed, {}, std::move(error)
            };
        }

        window_ = window;
        initialized_ = true;
        return {};
    }

    [[nodiscard]] RenderResult render(const RenderFramePacket& frame) {
        if (!initialized_)
            return {RenderError::NotInitialized, {}, "RenderDevice is not initialized"};

        platform::WindowMetrics metrics{};
        if (!surfaces_.refresh_surface_metrics(surface_id_, metrics)) {
            return {
                RenderError::SurfaceUnavailable, {},
                "Platform could not refresh native surface metrics"
            };
        }
        if (metrics.minimized ||
            metrics.pixel_width == 0U || metrics.pixel_height == 0U) {
            return {
                RenderError::SurfaceSuspended, {},
                "Native surface is minimized or has zero pixel extent"
            };
        }

        const RenderSurfaceInfo latest = make_surface_info(metrics);
        if (!same_surface_size(latest, surface_info_)) {
            std::string resize_error;
            if (!backend_.resize(latest, resize_error)) {
                return {
                    RenderError::BackendResizeFailed, {}, std::move(resize_error)
                };
            }
            surface_info_ = latest;
        } else {
            surface_info_ = latest;
        }

        if (frame.framebuffer_extent != surface_info_.pixel_extent) {
            return {
                RenderError::InvalidFrame, FrameValidationError::InvalidExtent,
                "Frame extent does not match the current framebuffer size"
            };
        }

        const auto validation = validate_frame_packet(frame, capabilities_);
        if (validation != FrameValidationError::None) {
            return {
                RenderError::InvalidFrame, validation,
                "RenderFramePacket failed validation"
            };
        }

        std::string error;
        if (!backend_.render(frame, error)) {
            return {RenderError::BackendRenderFailed, {}, std::move(error)};
        }
        return {};
    }

    void shutdown() noexcept {
        if (initialized_) backend_.shutdown();
        initialized_ = false;
        capabilities_ = {};
        surface_info_ = {};
        window_ = {};
        reset_surface_lease();
    }

private:
    [[nodiscard]] static RenderSurfaceInfo make_surface_info(
        const platform::WindowMetrics& metrics) noexcept {
        return {
            {metrics.pixel_width, metrics.pixel_height},
            {metrics.logical_width, metrics.logical_height},
            metrics.scale_x, metrics.scale_y, metrics.focused
        };
    }

    [[nodiscard]] static bool same_surface_size(
        const RenderSurfaceInfo& left,
        const RenderSurfaceInfo& right) noexcept {
        return left.pixel_extent == right.pixel_extent;
    }

    void reset_surface_lease() noexcept {
        if (surface_id_) surfaces_.release_surface(surface_id_);
        surface_id_ = {};
    }

    platform::ISurfaceSystem& surfaces_;
    IRenderBackend& backend_;
    platform::WindowId window_{};
    platform::SurfaceId surface_id_{};
    RenderSurfaceInfo surface_info_{};
    RenderCapabilities capabilities_{};
    bool initialized_{false};
};

} // namespace dream::render
