#include <dream/runtime/engine_runtime.hpp>

#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace {
using namespace dream::runtime;
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
    WindowMetrics metrics{640U, 480U, 1280U, 960U, 2.0F, 2.0F, true, false};
    int acquired{0};
    int released{0};
    bool acquire_ok{true};

    bool acquire_surface(
        WindowId window, SurfaceId& result, NativeSurface& native,
        WindowMetrics& output_metrics, std::string& error) override {
        if (!acquire_ok || !window) {
            error = "fake surface acquire failed";
            return false;
        }
        ++acquired;
        result = {91U};
        native.kind = NativeSurfaceKind::Headless;
        native.window = reinterpret_cast<void*>(0x1);
        output_metrics = metrics;
        return true;
    }

    void release_surface(SurfaceId surface) noexcept override {
        if (surface) ++released;
    }

    bool refresh_surface_metrics(
        SurfaceId surface, WindowMetrics& output_metrics) const noexcept override {
        if (!surface) return false;
        output_metrics = metrics;
        return true;
    }
};

class FakeRenderBackend final : public IRenderBackend {
public:
    bool initialize_ok{true};
    int initialize_calls{0};
    int resize_calls{0};
    int render_calls{0};
    int shutdown_calls{0};
    std::size_t last_2d_draws{0};
    std::size_t last_3d_draws{0};

    bool initialize(
        NativeSurface, const RenderSurfaceInfo&,
        RenderCapabilities& capabilities, std::string& error) override {
        ++initialize_calls;
        if (!initialize_ok) {
            error = "fake backend init failed";
            return false;
        }
        capabilities = {true, true, true, 1000U, 8192U};
        return true;
    }

    bool resize(const RenderSurfaceInfo&, std::string&) override {
        ++resize_calls;
        return true;
    }

    bool render(const RenderFramePacket& frame, std::string&) override {
        ++render_calls;
        last_2d_draws = frame.draws_2d.size();
        last_3d_draws = frame.draws_3d.size();
        return true;
    }

    void shutdown() noexcept override { ++shutdown_calls; }
};

Matrix3x3 translation(float x, float y) {
    Matrix3x3 matrix;
    matrix.at(0, 2) = x;
    matrix.at(1, 2) = y;
    return matrix;
}
}

int main() {
    FakeSurfaceSystem surfaces;
    FakeRenderBackend backend;
    EngineRuntimeConfig config;
    config.scene_name = "Runtime test";
    config.fixed_step = {0.1, 0.3, 4};
    config.physics2d.gravity = {0.0F, 0.0F};
    config.physics2d.maximum_delta_seconds = 0.2F;

    EngineRuntime runtime(surfaces, backend, config);
    require(runtime.configuration_valid(), "runtime configuration validates");
    require(runtime.state() == RuntimeState::Created,
            "runtime begins in Created state");
    require(static_cast<bool>(runtime.initialize({5U})),
            "runtime initializes its renderer and platform surface");
    require(runtime.state() == RuntimeState::Initialized &&
            surfaces.acquired == 1 && backend.initialize_calls == 1,
            "runtime transitions to Initialized once");

    require(runtime.actions().add_action("jump") == input::ActionMapError::None,
            "runtime action map accepts gameplay actions");
    require(runtime.actions().bind_key("jump", 0x2CU) ==
                input::ActionMapError::None,
            "runtime action map accepts HID keyboard bindings");

    const auto body = runtime.physics2d().create_body(
        physics2d::BodyDescription2D{
            .type = physics2d::BodyType::Dynamic,
            .position = {10.0F, 0.0F},
            .linear_velocity = {1.0F, 0.0F},
            .mass = 1.0F,
            .collider = physics2d::CircleCollider2D{{}, 0.25F}
        });
    require(static_cast<bool>(body), "runtime physics world accepts dynamic bodies");

    std::vector<std::string> phases;
    std::size_t observed_jump_ticks = 0;
    const auto pre = runtime.scene().systems().add_system(
        "game.pre", SystemPhase::PreUpdate, 0,
        [&phases](World&, const SystemContext&) { phases.emplace_back("pre"); });
    const auto update = runtime.scene().systems().add_system(
        "game.update", SystemPhase::Update, 0,
        [&runtime, &phases, &observed_jump_ticks]
        (World&, const SystemContext& context) {
            phases.emplace_back("update");
            require(context.delta_seconds == 0.1F,
                    "gameplay system receives the configured fixed timestep");
            const auto state = runtime.actions().state("jump");
            if (state && state->just_pressed) ++observed_jump_ticks;
        });
    const auto post = runtime.scene().systems().add_system(
        "game.post", SystemPhase::PostUpdate, 0,
        [&phases](World&, const SystemContext&) { phases.emplace_back("post"); });
    require(pre && update && post, "runtime scene accepts application systems");
    require(static_cast<bool>(runtime.physics_system()),
            "runtime registers its built-in PhysicsWorld2D system");

    InputEvent key_down;
    key_down.type = InputEventType::KeyDown;
    key_down.key_code = 0x2CU;
    key_down.timestamp_ns = 100U;
    const std::array<InputEvent, 1> first_events{key_down};
    const auto first_update = runtime.update(0.25, first_events);
    require(static_cast<bool>(first_update) &&
            first_update.fixed_step.steps_executed == 2U &&
            first_update.fixed_step.tick_index == 2U,
            "runtime advances two fixed simulation steps");
    require(first_update.input_events_handled == 1U,
            "runtime forwards platform input into ActionMap");
    require(observed_jump_ticks == 2U,
            "input snapshot remains visible during every fixed tick in the frame");
    require(phases == std::vector<std::string>{
                "pre", "update", "post", "pre", "update", "post"},
            "scene systems execute in PreUpdate, Update, PostUpdate order per tick");

    const auto moved = runtime.physics2d().body_state(body.handle);
    require(moved && std::abs(moved->position.x - 10.2F) < 0.001F,
            "built-in physics system advances bodies at the fixed simulation rate");

    require(static_cast<bool>(runtime.begin_render_frame(
                {0.05F, 0.1F, 0.15F, 1.0F})),
            "runtime starts a render frame using the acquired framebuffer size");
    require(runtime.frame_builder().set_camera_2d(CameraMatrices2D{}) ==
                FrameBuilderError::None,
            "runtime frame builder accepts a 2D camera");
    SpriteQuad2D sprite;
    sprite.transform = translation(2.0F, 3.0F);
    sprite.tint = {1.0F, 0.5F, 0.25F, 1.0F};
    require(runtime.frame_builder().add_quad(sprite) == FrameBuilderError::None,
            "runtime frame builder accepts a sprite");
    require(static_cast<bool>(runtime.render_frame()),
            "runtime finalizes and renders the frame");
    require(backend.render_calls == 1 && backend.last_2d_draws == 1U,
            "render backend receives validated frame commands");

    InputEvent key_up = key_down;
    key_up.type = InputEventType::KeyUp;
    key_up.timestamp_ns = 200U;
    const std::array<InputEvent, 1> second_events{key_up};
    const auto second_update = runtime.update(0.05, second_events);
    require(static_cast<bool>(second_update) &&
            second_update.fixed_step.steps_executed == 1U &&
            second_update.fixed_step.tick_index == 3U,
            "runtime preserves fractional frame time between rendered frames");
    const auto jump = runtime.actions().state("jump");
    require(jump && !jump->down && jump->just_released,
            "runtime resets frame edges and processes the release event");

    require(static_cast<bool>(runtime.begin_render_frame()),
            "runtime can begin another empty frame");
    require(static_cast<bool>(runtime.render_frame()),
            "runtime renders an empty frame");
    require(backend.render_calls == 2 && backend.last_2d_draws == 0U,
            "runtime clears and presents a frame without geometry");

    const auto invalid_delta = runtime.update(-0.1);
    require(invalid_delta.error == RuntimeError::InvalidFrameDelta,
            "runtime rejects negative render delta before mutating input state");

    const auto duplicate_init = runtime.initialize({5U});
    require(duplicate_init.error == RuntimeError::AlreadyInitialized,
            "runtime rejects duplicate initialization");

    runtime.shutdown();
    require(runtime.state() == RuntimeState::Stopped &&
            surfaces.released == 1 &&
            backend.shutdown_calls == 1,
            "runtime stops the renderer before releasing its platform surface");
    runtime.shutdown();
    require(surfaces.released == 1,
            "runtime shutdown is idempotent");

    FakeSurfaceSystem resize_surfaces;
    FakeRenderBackend resize_backend;
    EngineRuntime resize_runtime(resize_surfaces, resize_backend, config);
    require(static_cast<bool>(resize_runtime.initialize({17U})),
            "resize runtime initializes");
    resize_surfaces.metrics.pixel_width = 1600U;
    resize_surfaces.metrics.pixel_height = 1200U;
    require(static_cast<bool>(resize_runtime.begin_render_frame()),
            "runtime refreshes the surface before building a resized frame");
    require(resize_backend.resize_calls == 1 &&
            resize_runtime.frame_builder().finalize() == FrameBuilderError::None,
            "backend resized before frame packet construction");
    require(resize_runtime.frame_builder().packet().framebuffer_extent ==
                Extent2D{1600U, 1200U},
            "new frame packet uses the new physical framebuffer extent");
    require(static_cast<bool>(resize_runtime.render_frame()),
            "resized frame renders without a stale-extent failure");
    resize_runtime.shutdown();

    FakeSurfaceSystem invalid_surfaces;
    FakeRenderBackend invalid_backend;
    EngineRuntimeConfig invalid_config;
    invalid_config.fixed_step = {0.1, 0.3, 4};
    invalid_config.physics2d.maximum_delta_seconds = 0.05F;
    EngineRuntime invalid_runtime(invalid_surfaces, invalid_backend, invalid_config);
    require(!invalid_runtime.configuration_valid(),
            "runtime rejects fixed timesteps larger than physics maximum delta");
    require(invalid_runtime.initialize({9U}).error ==
                RuntimeError::InvalidConfiguration &&
            invalid_surfaces.acquired == 0,
            "invalid runtime config fails before acquiring a native surface");

    std::cout << "Dream Engine runtime integration tests passed.\n";
    return EXIT_SUCCESS;
}
