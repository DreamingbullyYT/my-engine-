#pragma once

#include <dream/platform/surface.hpp>
#include <dream/render/render_backend.hpp>
#include <dream/ui/ui_runtime.hpp>

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

namespace dream::examples {

class HeadlessHost final : public platform::ISurfaceSystem,
                           public render::IRenderBackend {
public:
    platform::WindowMetrics metrics{800U, 600U, 800U, 600U, 1.0F, 1.0F, true, false};
    std::size_t frames{0};
    std::size_t last_2d_draws{0};
    std::size_t last_3d_draws{0};
    std::size_t uploaded_meshes{0};
    std::size_t uploaded_materials{0};
    bool surface_acquired{false};
    bool backend_initialized{false};

    bool acquire_surface(
        platform::WindowId window, platform::SurfaceId& result,
        platform::NativeSurface& native, platform::WindowMetrics& output,
        std::string& error) override {
        if (!window || surface_acquired) {
            error = "headless host requires a valid, unclaimed window";
            return false;
        }
        surface_acquired = true;
        result = {1U};
        native.kind = platform::NativeSurfaceKind::Headless;
        native.window = this;
        output = metrics;
        return true;
    }

    void release_surface(platform::SurfaceId surface) noexcept override {
        if (surface.value == 1U) surface_acquired = false;
    }

    bool refresh_surface_metrics(
        platform::SurfaceId surface,
        platform::WindowMetrics& output) const noexcept override {
        if (surface.value != 1U || !surface_acquired) return false;
        output = metrics;
        return true;
    }

    bool initialize(
        platform::NativeSurface native, const render::RenderSurfaceInfo&,
        render::RenderCapabilities& capabilities, std::string& error) override {
        if (native.kind != platform::NativeSurfaceKind::Headless ||
            native.window != this || backend_initialized) {
            error = "invalid or already initialized headless render host";
            return false;
        }
        backend_initialized = true;
        capabilities = {true, true, false, 4096U, 4096U};
        return true;
    }

    bool resize(const render::RenderSurfaceInfo&, std::string&) override {
        return backend_initialized;
    }

    bool render(const render::RenderFramePacket& frame, std::string& error) override {
        if (!backend_initialized) {
            error = "headless renderer is not initialized";
            return false;
        }
        ++frames;
        last_2d_draws = frame.draws_2d.size();
        last_3d_draws = frame.draws_3d.size();
        return true;
    }

    bool create_mesh(
        render::RenderAssetId id, const render::MeshDescription& mesh,
        std::string& error) override {
        if (!backend_initialized || id == 0U || mesh.vertices.empty() ||
            mesh.indices.empty()) {
            error = "invalid headless mesh upload";
            return false;
        }
        ++uploaded_meshes;
        return true;
    }

    bool destroy_mesh(render::RenderAssetId id, std::string& error) override {
        if (!backend_initialized || id == 0U) {
            error = "invalid headless mesh release";
            return false;
        }
        if (uploaded_meshes > 0U) --uploaded_meshes;
        return true;
    }

    bool create_material(
        render::RenderAssetId id, const render::MaterialDescription&,
        std::string& error) override {
        if (!backend_initialized || id == 0U) {
            error = "invalid headless material creation";
            return false;
        }
        ++uploaded_materials;
        return true;
    }

    bool destroy_material(
        render::RenderAssetId id, std::string& error) override {
        if (!backend_initialized || id == 0U) {
            error = "invalid headless material release";
            return false;
        }
        if (uploaded_materials > 0U) --uploaded_materials;
        return true;
    }

    void shutdown() noexcept override {
        backend_initialized = false;
        uploaded_meshes = 0U;
        uploaded_materials = 0U;
    }
};

class BasicUiMetrics final : public ui::IUiTextMetrics {
public:
    ui::UiVec2 measure(std::string_view text, float font_size) const noexcept override {
        return {static_cast<float>(text.size()) * font_size * 0.5F, font_size};
    }
};

} // namespace dream::examples
