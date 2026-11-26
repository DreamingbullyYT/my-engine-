#pragma once

#include <dream/platform/surface.hpp>
#include <dream/render/render_types.hpp>

#include <cstddef>
#include <limits>
#include <span>
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
    BackendRenderFailed,
    InvalidResource,
    BackendResourceFailed
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

    // RGBA8 texture upload. Asset ID zero is reserved for backend-owned white
    // texture/default material and cannot be registered by callers.
    [[nodiscard]] virtual bool create_texture_rgba8(
        RenderAssetId texture_id, Extent2D extent,
        std::span<const std::byte> pixels, std::string& error) {
        (void)texture_id;
        (void)extent;
        (void)pixels;
        error = "Render backend does not support texture uploads";
        return false;
    }

    [[nodiscard]] virtual bool destroy_texture(
        RenderAssetId texture_id, std::string& error) {
        (void)texture_id;
        error = "Render backend does not support texture destruction";
        return false;
    }

    [[nodiscard]] virtual bool create_mesh(
        RenderAssetId mesh_id, const MeshDescription& mesh,
        std::string& error) {
        (void)mesh_id; (void)mesh;
        error = "Render backend does not support mesh uploads";
        return false;
    }

    [[nodiscard]] virtual bool destroy_mesh(
        RenderAssetId mesh_id, std::string& error) {
        (void)mesh_id;
        error = "Render backend does not support mesh destruction";
        return false;
    }

    [[nodiscard]] virtual bool create_material(
        RenderAssetId material_id, const MaterialDescription& material,
        std::string& error) {
        (void)material_id; (void)material;
        error = "Render backend does not support material creation";
        return false;
    }

    [[nodiscard]] virtual bool destroy_material(
        RenderAssetId material_id, std::string& error) {
        (void)material_id;
        error = "Render backend does not support material destruction";
        return false;
    }

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

    // Refreshes the current framebuffer metrics and performs backend resize
    // before callers build a frame packet for the new extent.
    [[nodiscard]] RenderResult refresh_surface() {
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
        }
        surface_info_ = latest;
        return {};
    }

    [[nodiscard]] RenderResult create_texture_rgba8(
        RenderAssetId texture_id, Extent2D extent,
        std::span<const std::byte> pixels) {
        if (!initialized_)
            return {RenderError::NotInitialized, {}, "RenderDevice is not initialized"};
        if (texture_id == 0U || !extent.valid() ||
            extent.width > std::numeric_limits<std::size_t>::max() / 4U / extent.height) {
            return {
                RenderError::InvalidResource, {},
                "Texture ID and RGBA8 dimensions must be valid"
            };
        }
        const std::size_t expected_bytes =
            static_cast<std::size_t>(extent.width) *
            static_cast<std::size_t>(extent.height) * 4U;
        if (pixels.size() != expected_bytes ||
            pixels.size() > std::numeric_limits<std::uint32_t>::max()) {
            return {
                RenderError::InvalidResource, {},
                "RGBA8 texture data size does not match the requested extent"
            };
        }

        std::string error;
        if (!backend_.create_texture_rgba8(texture_id, extent, pixels, error)) {
            return {RenderError::BackendResourceFailed, {}, std::move(error)};
        }
        return {};
    }

    [[nodiscard]] RenderResult destroy_texture(RenderAssetId texture_id) {
        if (!initialized_)
            return {RenderError::NotInitialized, {}, "RenderDevice is not initialized"};
        if (texture_id == 0U)
            return {
                RenderError::InvalidResource, {},
                "Texture ID zero is reserved for the backend white texture"
            };

        std::string error;
        if (!backend_.destroy_texture(texture_id, error))
            return {RenderError::BackendResourceFailed, {}, std::move(error)};
        return {};
    }

    [[nodiscard]] RenderResult create_mesh(
        RenderAssetId mesh_id, const MeshDescription& mesh) {
        if (!initialized_)
            return {RenderError::NotInitialized, {}, "RenderDevice is not initialized"};
        const auto validation = validate_mesh_description(mesh);
        if (mesh_id == 0U || validation != FrameValidationError::None)
            return {RenderError::InvalidResource, validation,
                    "Mesh ID, vertices or indices are invalid"};
        std::string error;
        if (!backend_.create_mesh(mesh_id, mesh, error))
            return {RenderError::BackendResourceFailed, {}, std::move(error)};
        return {};
    }

    [[nodiscard]] RenderResult destroy_mesh(RenderAssetId mesh_id) {
        if (!initialized_)
            return {RenderError::NotInitialized, {}, "RenderDevice is not initialized"};
        if (mesh_id == 0U)
            return {RenderError::InvalidResource, {}, "Mesh ID zero is reserved"};
        std::string error;
        if (!backend_.destroy_mesh(mesh_id, error))
            return {RenderError::BackendResourceFailed, {}, std::move(error)};
        return {};
    }

    [[nodiscard]] RenderResult create_material(
        RenderAssetId material_id, const MaterialDescription& material) {
        if (!initialized_)
            return {RenderError::NotInitialized, {}, "RenderDevice is not initialized"};
        if (material_id == 0U || !valid_material_description(material))
            return {RenderError::InvalidResource, {}, "Material ID or properties are invalid"};
        std::string error;
        if (!backend_.create_material(material_id, material, error))
            return {RenderError::BackendResourceFailed, {}, std::move(error)};
        return {};
    }

    [[nodiscard]] RenderResult destroy_material(RenderAssetId material_id) {
        if (!initialized_)
            return {RenderError::NotInitialized, {}, "RenderDevice is not initialized"};
        if (material_id == 0U)
            return {RenderError::InvalidResource, {}, "Material ID zero is reserved"};
        std::string error;
        if (!backend_.destroy_material(material_id, error))
            return {RenderError::BackendResourceFailed, {}, std::move(error)};
        return {};
    }

    [[nodiscard]] RenderResult render(const RenderFramePacket& frame) {
        const auto refreshed = refresh_surface();
        if (!refreshed) return refreshed;

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
