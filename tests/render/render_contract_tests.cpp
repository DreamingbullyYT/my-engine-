#include <dream/render/render_backend.hpp>

#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace {
using namespace dream::render;
using namespace dream::platform;
using namespace dream::world;

void require(bool condition, std::string_view message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class FakeSurfaceSystem final : public ISurfaceSystem {
public:
    bool acquire_ok{true};
    bool refresh_ok{true};
    WindowMetrics metrics{640, 480, 1280, 960, 2.0F, 2.0F, true, false};
    int acquired{0};
    int released{0};

    bool acquire_surface(
        WindowId window, SurfaceId& result, NativeSurface& native,
        WindowMetrics& output_metrics, std::string& error) override {
        if (!acquire_ok || !window) {
            error = "fake acquire failed";
            return false;
        }
        ++acquired;
        result = {77U};
        native.kind = NativeSurfaceKind::Headless;
        output_metrics = metrics;
        return true;
    }

    void release_surface(SurfaceId surface) noexcept override {
        if (surface) ++released;
    }

    bool refresh_surface_metrics(
        SurfaceId surface, WindowMetrics& output_metrics) const noexcept override {
        if (!refresh_ok || !surface) return false;
        output_metrics = metrics;
        return true;
    }
};

class FakeRenderBackend final : public IRenderBackend {
public:
    bool initialize_ok{true};
    bool resize_ok{true};
    bool render_ok{true};
    int initialize_calls{0};
    int resize_calls{0};
    int render_calls{0};
    int shutdown_calls{0};
    RenderSurfaceInfo last_surface{};
    std::string error_message{"fake backend failure"};

    bool initialize(
        NativeSurface native, const RenderSurfaceInfo& surface,
        RenderCapabilities& output_capabilities, std::string& error) override {
        ++initialize_calls;
        if (native.kind != NativeSurfaceKind::Headless) {
            error = "unexpected native surface kind";
            return false;
        }
        if (!initialize_ok) {
            error = error_message;
            return false;
        }
        last_surface = surface;
        output_capabilities = {true, true, true, 1000U, 8192U};
        return true;
    }

    bool resize(const RenderSurfaceInfo& surface, std::string& error) override {
        ++resize_calls;
        if (!resize_ok) {
            error = error_message;
            return false;
        }
        last_surface = surface;
        return true;
    }

    bool render(const RenderFramePacket&, std::string& error) override {
        ++render_calls;
        if (!render_ok) {
            error = error_message;
            return false;
        }
        return true;
    }

    void shutdown() noexcept override { ++shutdown_calls; }
};

RenderFramePacket valid_empty_frame(Extent2D extent) {
    RenderFramePacket frame;
    frame.delta_seconds = 1.0 / 60.0;
    frame.framebuffer_extent = extent;
    return frame;
}

CameraMatrices2D identity_camera_2d() {
    CameraMatrices2D camera;
    return camera;
}

CameraMatrices3D identity_camera_3d() {
    CameraMatrices3D camera;
    return camera;
}
}

int main() {
    RenderCapabilities caps{true, true, true, 16U, 4096U};
    auto frame = valid_empty_frame({1280U, 720U});
    require(validate_frame_packet(frame, caps) == FrameValidationError::None,
            "empty frame with a valid extent is valid");

    frame.framebuffer_extent = {};
    require(validate_frame_packet(frame, caps) == FrameValidationError::InvalidExtent,
            "zero framebuffer extent is rejected");
    frame = valid_empty_frame({1280U, 720U});
    frame.delta_seconds = -1.0;
    require(validate_frame_packet(frame, caps) == FrameValidationError::InvalidDelta,
            "negative frame delta is rejected");

    frame = valid_empty_frame({1280U, 720U});
    frame.draws_2d = std::span<const DrawCommand2D>{
        std::array<DrawCommand2D, 1>{DrawCommand2D{}}};
    require(validate_frame_packet(frame, caps) == FrameValidationError::MissingCamera,
            "2D draws require a 2D camera");

    std::array<Vertex2D, 3> vertices{{
        {{0, 0}, {0, 0}, {1, 1, 1, 1}},
        {{1, 0}, {1, 0}, {1, 1, 1, 1}},
        {{0, 1}, {0, 1}, {1, 1, 1, 1}}
    }};
    std::array<std::uint32_t, 3> indices{0U, 1U, 2U};
    std::array<DrawCommand2D, 1> draws2d{{
        DrawCommand2D{0U, 3U, 0U, 0U, Matrix3x3{}, {1, 1, 1, 1}, 0, 0, BlendMode::Alpha}
    }};
    frame = valid_empty_frame({1280U, 720U});
    frame.camera_2d = identity_camera_2d();
    frame.vertices_2d = vertices;
    frame.indices_2d = indices;
    frame.draws_2d = draws2d;
    require(validate_frame_packet(frame, caps) == FrameValidationError::None,
            "valid indexed 2D triangle is accepted");

    std::array<std::uint32_t, 3> bad_indices{0U, 1U, 3U};
    frame.indices_2d = bad_indices;
    require(validate_frame_packet(frame, caps) == FrameValidationError::InvalidIndexRange,
            "2D vertex index outside vertex span is rejected");

    frame.indices_2d = indices;
    draws2d[0].index_count = 2U;
    require(validate_frame_packet(frame, caps) == FrameValidationError::InvalidIndexRange,
            "non-triangle index count is rejected");
    draws2d[0].index_count = 3U;

    RenderCapabilities no_2d{false, true, false, 16U, 4096U};
    require(validate_frame_packet(frame, no_2d) == FrameValidationError::Unsupported2D,
            "backend without 2D support rejects 2D draws");

    std::array<DrawCommand3D, 1> draws3d{{
        DrawCommand3D{11U, 21U, Matrix4x4{}, {1, 1, 1, 1}, 0U}
    }};
    frame = valid_empty_frame({1280U, 720U});
    frame.camera_3d = identity_camera_3d();
    frame.draws_3d = draws3d;
    require(validate_frame_packet(frame, caps) == FrameValidationError::None,
            "valid 3D mesh/material draw is accepted");
    draws3d[0].mesh_id = 0U;
    require(validate_frame_packet(frame, caps) == FrameValidationError::InvalidMeshId,
            "zero mesh asset identifier is rejected");
    draws3d[0].mesh_id = 11U;

    FakeSurfaceSystem surfaces;
    FakeRenderBackend backend;
    {
        RenderDevice device(surfaces, backend);
        require(device.initialize({5U}), "render device initializes");
        require(device.initialized() && surfaces.acquired == 1 &&
                backend.initialize_calls == 1,
                "device acquires surface before backend initialization");
        require(device.capabilities().supports_2d &&
                device.capabilities().supports_3d,
                "device exposes both 2D and 3D backend capabilities");
        require(!device.initialize({5U}) &&
                device.render(valid_empty_frame({1280U, 720U})).error ==
                    RenderError::None,
                "already initialized device rejects second init but renders valid frame");

        surfaces.metrics.pixel_width = 1600U;
        surfaces.metrics.pixel_height = 900U;
        auto resized_frame = valid_empty_frame({1600U, 900U});
        require(device.render(resized_frame) &&
                backend.resize_calls == 1 &&
                backend.last_surface.pixel_extent == Extent2D{1600U, 900U},
                "device resizes backend when physical framebuffer changes");

        surfaces.metrics.minimized = true;
        require(device.render(resized_frame).error == RenderError::SurfaceSuspended,
                "minimized surface suspends rendering");
        surfaces.metrics.minimized = false;
        surfaces.refresh_ok = false;
        require(device.render(resized_frame).error == RenderError::SurfaceUnavailable,
                "failed surface metrics refresh prevents backend rendering");
        surfaces.refresh_ok = true;

        backend.render_ok = false;
        require(device.render(resized_frame).error == RenderError::BackendRenderFailed,
                "backend rendering error is returned");
        backend.render_ok = true;

        auto mismatch = valid_empty_frame({1280U, 720U});
        require(device.render(mismatch).error == RenderError::InvalidFrame,
                "frame with stale framebuffer extent is rejected");

        device.shutdown();
        require(!device.initialized() && surfaces.released == 1 &&
                backend.shutdown_calls == 1,
                "shutdown stops backend before releasing acquired surface");
        device.shutdown();
        require(surfaces.released == 1,
                "repeated shutdown does not double release surface");
    }

    FakeSurfaceSystem failed_surfaces;
    FakeRenderBackend failed_backend;
    failed_backend.initialize_ok = false;
    {
        RenderDevice failed_device(failed_surfaces, failed_backend);
        const auto failed = failed_device.initialize({6U});
        require(failed.error == RenderError::BackendInitializationFailed,
                "backend initialization failure is surfaced");
        require(failed_surfaces.released == 1 &&
                failed_backend.shutdown_calls == 1 &&
                !failed_device.initialized(),
                "failed initialization releases the surface lease");
    }

    FakeSurfaceSystem minimized_surfaces;
    minimized_surfaces.metrics.minimized = true;
    FakeRenderBackend unused_backend;
    RenderDevice minimized_device(minimized_surfaces, unused_backend);
    require(minimized_device.initialize({7U}).error == RenderError::SurfaceUnavailable,
            "minimized window cannot initialize a drawable backend");
    require(minimized_surfaces.released == 1 &&
            unused_backend.initialize_calls == 0,
            "invalid surface is released without backend initialization");

    FakeSurfaceSystem failed_acquire_surfaces;
    failed_acquire_surfaces.acquire_ok = false;
    FakeRenderBackend never_started_backend;
    RenderDevice never_started(failed_acquire_surfaces, never_started_backend);
    require(never_started.initialize({8U}).error ==
                RenderError::SurfaceAcquisitionFailed,
            "surface acquisition failure is surfaced");

    std::cout << "Dream Engine render contract tests passed.\n";
    return EXIT_SUCCESS;
}
