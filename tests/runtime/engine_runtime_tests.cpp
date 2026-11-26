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
using namespace dream;
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
    Color last_clear_color{};

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
        last_clear_color = frame.clear_color;
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

struct UpdateHookProbe final {
    EngineRuntime* expected_runtime{};
    std::size_t calls{0};
};
struct PhysicsHookProbe final { std::size_t calls{0}; };
struct RenderHookProbe final {
    std::size_t begin_calls{0};
    std::size_t render_calls{0};
};

void runtime_update_before(void* context, dmod::NativeHookInvocationV1* invocation) noexcept {
    if (!context || !invocation || !invocation->user_data) return;
    auto& probe = *static_cast<UpdateHookProbe*>(context);
    auto& call = *static_cast<RuntimeUpdateHookDataV1*>(invocation->user_data);
    if (call.runtime == probe.expected_runtime && probe.calls == 0)
        call.frame_delta_seconds = 0.26;
}
void runtime_update_after(void* context, dmod::NativeHookInvocationV1*) noexcept {
    if (context) ++static_cast<UpdateHookProbe*>(context)->calls;
}
void physics_step_after(void* context, dmod::NativeHookInvocationV1* invocation) noexcept {
    if (!context || !invocation || !invocation->user_data) return;
    const auto& call =
        *static_cast<RuntimePhysics2DStepHookDataV1*>(invocation->user_data);
    if (call.report.body_count != 0U)
        ++static_cast<PhysicsHookProbe*>(context)->calls;
}
void begin_render_before(void* context, dmod::NativeHookInvocationV1* invocation) noexcept {
    if (!context || !invocation || !invocation->user_data) return;
    auto& probe = *static_cast<RenderHookProbe*>(context);
    auto& call = *static_cast<RuntimeBeginRenderFrameHookDataV1*>(invocation->user_data);
    if (probe.begin_calls == 0)
        call.clear_color = {0.2F, 0.3F, 0.4F, 1.0F};
    ++probe.begin_calls;
}
void render_frame_after(void* context, dmod::NativeHookInvocationV1*) noexcept {
    if (context) ++static_cast<RenderHookProbe*>(context)->render_calls;
}
struct SceneSystemHookProbe final {
    std::size_t bypassed_calls{0};
};
void replace_scene_system(void* context, dmod::NativeHookInvocationV1* invocation) noexcept {
    if (!context || !invocation || !invocation->user_data) return;
    auto& probe = *static_cast<SceneSystemHookProbe*>(context);
    auto& call = *static_cast<RuntimeSystemHookDataV1*>(invocation->user_data);
    if (call.system.name == "game.hooked") {
        ++probe.bypassed_calls;
        return; // Intentionally do not invoke call_next: replace this system.
    }
    if (invocation->call_next)
        (void)invocation->call_next(invocation->next_context);
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
    const auto mod_api = runtime.mod_host().api();
    require(mod_api.query_interface(mod_api.host_context, "dream.engine.runtime", 1) == &runtime,
            "native mods receive the actual EngineRuntime object");
    require(mod_api.query_interface(mod_api.host_context, "dream.engine.world", 1) ==
                &runtime.scene().world(),
            "native mods receive the actual ECS World");
    require(mod_api.query_interface(mod_api.host_context, "dream.engine.physics2d", 1) ==
                &runtime.physics2d(),
            "native mods receive the actual PhysicsWorld2D");
    require(mod_api.query_interface(mod_api.host_context, "dream.engine.physics3d", 1) ==
                &runtime.physics3d(),
            "native mods receive the actual PhysicsWorld3D");
    require(mod_api.query_interface(mod_api.host_context, "dream.engine.animation", 1) ==
                &runtime.animations(),
            "native mods receive the actual animation runtime");
    require(mod_api.query_interface(mod_api.host_context, "dream.engine.audio", 1) ==
                &runtime.audio(),
            "native mods receive the actual audio mixer");
    require(mod_api.query_interface(mod_api.host_context, "dream.engine.ui", 1) ==
                &runtime.ui(),
            "native mods receive the actual retained UI runtime");
    require(mod_api.query_interface(mod_api.host_context, "dream.engine.assets.importers", 1) ==
                &runtime.asset_importers(),
            "native mods receive the actual asset importer registry");
    require(runtime.mods().loaded_count() == 0,
            "runtime owns the native mod manager");
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
    std::size_t suppressed_system_body_calls = 0;
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
    const auto hooked_system = runtime.scene().systems().add_system(
        "game.hooked", SystemPhase::Update, -1,
        [&suppressed_system_body_calls](World&, const SystemContext&) {
            ++suppressed_system_body_calls;
        });
    require(pre && update && post && hooked_system,
            "runtime scene accepts application systems");
    require(static_cast<bool>(runtime.physics_system()),
            "runtime registers its built-in PhysicsWorld2D system");

    UpdateHookProbe update_hook_probe{&runtime, 0};
    PhysicsHookProbe physics_hook_probe;
    RenderHookProbe render_hook_probe;
    SceneSystemHookProbe scene_system_hook_probe;
    auto& runtime_hooks = runtime.mods().hook_registry();
    const auto update_before = runtime_hooks.register_hook(
        "test.runtime", hook_points::runtime_update, dmod::NativeHookStage::Before,
        0, &runtime_update_before, &update_hook_probe);
    const auto update_after = runtime_hooks.register_hook(
        "test.runtime", hook_points::runtime_update, dmod::NativeHookStage::After,
        0, &runtime_update_after, &update_hook_probe);
    const auto physics_after = runtime_hooks.register_hook(
        "test.runtime", hook_points::physics2d_step, dmod::NativeHookStage::After,
        0, &physics_step_after, &physics_hook_probe);
    const auto begin_before = runtime_hooks.register_hook(
        "test.runtime", hook_points::begin_render_frame, dmod::NativeHookStage::Before,
        0, &begin_render_before, &render_hook_probe);
    const auto render_after = runtime_hooks.register_hook(
        "test.runtime", hook_points::render_frame, dmod::NativeHookStage::After,
        0, &render_frame_after, &render_hook_probe);
    const auto system_replace = runtime_hooks.register_hook(
        "test.runtime", hook_points::scene_system, dmod::NativeHookStage::Replace,
        0, &replace_scene_system, &scene_system_hook_probe);
    require(update_before && update_after && physics_after && begin_before &&
            render_after && system_replace,
            "game code can register native hooks at runtime extension points");

    InputEvent key_down;
    key_down.type = InputEventType::KeyDown;
    key_down.key_code = 0x2CU;
    key_down.timestamp_ns = 100U;
    const std::array<InputEvent, 1> first_events{key_down};
    const auto first_update = runtime.update(0.25, first_events);
    require(static_cast<bool>(first_update) &&
            first_update.fixed_step.steps_executed == 2U &&
            first_update.fixed_step.tick_index == 2U &&
            update_hook_probe.calls == 1U && physics_hook_probe.calls == 2U &&
            scene_system_hook_probe.bypassed_calls == 2U &&
            suppressed_system_body_calls == 0U,
            "runtime, physics steps and per-system scene hooks execute in the fixed loop");
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
    require(render_hook_probe.begin_calls == 1U,
            "begin-render hook can mutate the clear color before frame construction");
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
    require(backend.render_calls == 1 && backend.last_2d_draws == 1U &&
            backend.last_clear_color.r == 0.2F &&
            backend.last_clear_color.g == 0.3F &&
            backend.last_clear_color.b == 0.4F &&
            render_hook_probe.render_calls == 1U,
            "render hook contexts can mutate frame input and observe render completion");

    InputEvent key_up = key_down;
    key_up.type = InputEventType::KeyUp;
    key_up.timestamp_ns = 200U;
    const std::array<InputEvent, 1> second_events{key_up};
    const auto second_update = runtime.update(0.05, second_events);
    require(static_cast<bool>(second_update) &&
            second_update.fixed_step.steps_executed == 1U &&
            second_update.fixed_step.tick_index == 3U &&
            update_hook_probe.calls == 2U && physics_hook_probe.calls == 3U &&
            scene_system_hook_probe.bypassed_calls == 3U &&
            suppressed_system_body_calls == 0U,
            "scene system replace hooks run for each fixed simulation tick");
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

    for (const auto registration : {update_before, update_after, physics_after,
                                    begin_before, render_after, system_replace})
        require(runtime_hooks.remove_hook(registration),
                "remove runtime test hook registration");
    // Runtime-owned service integration: native clients use the same live
    // systems that the fixed-step scene scheduler advances.
    auto& animation_service = runtime.animations();
    (void)animation_service;

    dream::physics3d::BodyDescription3D runtime_floor;
    runtime_floor.type = dream::physics3d::BodyType3D::Static;
    runtime_floor.position = {0.0F,-0.5F,0.0F};
    runtime_floor.collider = dream::physics3d::BoxCollider3D{
        {},{4.0F,0.5F,4.0F},{}};
    const auto runtime_floor_body = runtime.physics3d().create_body(runtime_floor);
    dream::physics3d::BodyDescription3D runtime_ball;
    runtime_ball.type = dream::physics3d::BodyType3D::Dynamic;
    runtime_ball.position = {0.0F,2.0F,0.0F};
    runtime_ball.collider = dream::physics3d::SphereCollider3D{{},0.5F};
    const auto runtime_ball_body = runtime.physics3d().create_body(runtime_ball);
    require(runtime_floor_body && runtime_ball_body,
            "EngineRuntime owns a working Physics3D world");
    for (unsigned step=0; step<180U; ++step) {
        const auto update = runtime.update(1.0/60.0,{});
        require(static_cast<bool>(update),
                "runtime fixed-step update advances animation and Physics3D");
    }
    const auto settled_ball = runtime.physics3d().body_state(runtime_ball_body.handle);
    require(settled_ball && settled_ball->position.y > 0.3F &&
            settled_ball->position.y < 0.9F,
            "runtime scheduler advances the 3D body against its floor");
    dream::audio::AudioClip runtime_audio_clip;
    runtime_audio_clip.sample_rate = 48000U;
    runtime_audio_clip.channels = 1U;
    runtime_audio_clip.samples = {0.25F,0.5F};
    const auto runtime_audio = runtime.audio().add_clip(runtime_audio_clip);
    require(runtime_audio.second == dream::audio::AudioError::None,
            "EngineRuntime owns an audio mixer");
    const auto runtime_voice = runtime.audio().play(runtime_audio.first);
    require(runtime_voice.second == dream::audio::AudioError::None,
            "runtime audio mixer starts a voice");
    std::array<float,4> runtime_audio_output{};
    require(runtime.audio().mix_stereo(runtime_audio_output,2U,48000U) ==
                dream::audio::AudioError::None &&
            runtime_audio_output[0] > 0.0F,
            "runtime audio mixer produces stereo PCM");
    require(static_cast<bool>(runtime.audio().remove_clip(runtime_audio.first)),
            "runtime audio clip releases after playback");

    std::string importer_id;
    dream::assets::AssetImporter importer;
    require(runtime.asset_importers().find(".obj",importer_id,importer) &&
            importer_id == "dream.builtin.obj",
            "EngineRuntime registers the built-in OBJ importer");

    dream::ui::UiNodeDescription runtime_panel;
    runtime_panel.type = dream::ui::UiNodeType::Panel;
    const auto runtime_ui_node = runtime.ui().create_node({},runtime_panel);
    require(static_cast<bool>(runtime_ui_node),
            "EngineRuntime owns a live retained UI tree");
    require(runtime.ui().remove_node(runtime_ui_node.handle),
            "runtime UI node can be destroyed cleanly");

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
