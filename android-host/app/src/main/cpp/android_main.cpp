#include <dream/core/log.hpp>
#include <dream/platform/android/game_activity_backend.hpp>
#include <dream/platform/application.hpp>
#include <dream/platform/window.hpp>
#include <dream/platform/android/android_asset_filesystem.hpp>
#include <dream/render/bgfx/bgfx_clear_backend.hpp>
#include <dream/render/bgfx/bgfx_shader_assets.hpp>
#include <dream/runtime/engine_runtime.hpp>
#include <dream/world/camera.hpp>
#include <dream/audio/android/aaudio_output.hpp>
#include <dream/ui/bitmap_font.hpp>

#if defined(__ANDROID__)
#include <android/native_activity.h>
#include <android/log.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

using dream::core::LogLevel;
using dream::core::Logger;

class HostUiMetrics final : public dream::ui::IUiTextMetrics {
public:
    [[nodiscard]] dream::ui::UiVec2 measure(
        std::string_view text, float font_size) const noexcept override {
        return {
            static_cast<float>(text.size()) * font_size * (6.0F / 7.0F),
            font_size
        };
    }
};

struct DemoState final {
    dream::physics2d::BodyHandle player_2d{};
    dream::physics3d::BodyHandle3D ball_3d{};
    dream::ui::UiNodeHandle root_panel{};
    dream::ui::UiNodeHandle status_panel{};
    dream::ui::UiNodeHandle player_name_field{};
    dream::ui::UiNodeHandle action_button{};
    bool scene_created{false};
    bool gpu_resources_created{false};
    std::uint32_t ui_width{0};
    std::uint32_t ui_height{0};
};

void log_result(
    Logger& logger, LogLevel level, std::string_view category,
    std::string message) noexcept {
    logger.write(level, category, message);
}

bool create_demo_scene(
    dream::runtime::EngineRuntime& engine,
    DemoState& demo,
    Logger& logger) {
    using namespace dream;

    physics2d::BodyDescription2D ground_2d;
    ground_2d.type = physics2d::BodyType::Static;
    ground_2d.position = {0.0F, -2.0F};
    ground_2d.collider =
        physics2d::BoxCollider2D{{}, {8.0F, 0.5F}, 0.0F};
    if (!engine.physics2d().create_body(ground_2d)) {
        log_result(logger, LogLevel::error, "physics2d",
                   "Could not create the Android demo ground body");
        return false;
    }

    physics2d::BodyDescription2D player;
    player.type = physics2d::BodyType::Dynamic;
    player.position = {-1.0F, 3.0F};
    player.collider = physics2d::CircleCollider2D{{}, 0.5F};
    const auto player_result = engine.physics2d().create_body(player);
    if (!player_result) {
        log_result(logger, LogLevel::error, "physics2d",
                   "Could not create the Android demo player body; create_error=" +
                   std::to_string(static_cast<unsigned>(player_result.error)));
        return false;
    }
    demo.player_2d = player_result.handle;

    physics3d::BodyDescription3D floor_3d;
    floor_3d.type = physics3d::BodyType3D::Static;
    floor_3d.position = {0.0F, -0.5F, 0.0F};
    floor_3d.collider =
        physics3d::BoxCollider3D{{}, {5.0F, 0.5F, 5.0F}, {}};
    if (!engine.physics3d().create_body(floor_3d)) {
        log_result(logger, LogLevel::error, "physics3d",
                   "Could not create the Android demo 3D floor");
        return false;
    }

    physics3d::BodyDescription3D ball;
    ball.type = physics3d::BodyType3D::Dynamic;
    ball.position = {1.0F, 2.5F, 0.0F};
    ball.collider = physics3d::SphereCollider3D{{}, 0.5F};
    const auto ball_result = engine.physics3d().create_body(ball);
    if (!ball_result) {
        log_result(logger, LogLevel::error, "physics3d",
                   "Could not create the Android demo 3D body; create_error=" +
                   std::to_string(static_cast<unsigned>(ball_result.error)));
        return false;
    }
    demo.ball_3d = ball_result.handle;

    ui::UiNodeDescription root;
    root.type = ui::UiNodeType::Panel;
    root.style.width = ui::UiSize::fill();
    root.style.height = ui::UiSize::fill();
    root.style.padding = 12.0F;
    root.style.align_items = ui::AlignItems::Start;
    root.style.background = {0.0F, 0.0F, 0.0F, 0.0F};
    const auto root_result = engine.ui().create_node({}, root);
    if (!root_result) {
        log_result(logger, LogLevel::error, "ui",
                   "Could not create the Android demo root UI node");
        return false;
    }
    demo.root_panel = root_result.handle;

    ui::UiNodeDescription panel;
    panel.type = ui::UiNodeType::Panel;
    panel.style.width = ui::UiSize::fixed(240.0F);
    panel.style.height = ui::UiSize::fixed(118.0F);
    panel.style.padding = 8.0F;
    panel.style.gap = 5.0F;
    panel.style.background = {0.035F, 0.055F, 0.09F, 0.92F};
    const auto panel_result = engine.ui().create_node(demo.root_panel, panel);
    if (!panel_result) {
        log_result(logger, LogLevel::error, "ui",
                   "Could not create the Android demo retained UI panel");
        return false;
    }
    demo.status_panel = panel_result.handle;

    ui::UiNodeDescription name_field;
    name_field.type = ui::UiNodeType::TextField;
    name_field.text = "";
    name_field.style.width = ui::UiSize::fill();
    name_field.style.height = ui::UiSize::fixed(30.0F);
    name_field.style.padding = 5.0F;
    name_field.style.background = {0.09F, 0.12F, 0.18F, 1.0F};
    name_field.style.foreground = {1.0F, 1.0F, 1.0F, 1.0F};
    const auto name_result = engine.ui().create_node(
        demo.status_panel, name_field);
    if (!name_result) {
        log_result(logger, LogLevel::error, "ui",
                   "Could not create the Android demo text field");
        return false;
    }
    demo.player_name_field = name_result.handle;

    ui::UiNodeDescription button;
    button.type = ui::UiNodeType::Button;
    button.text = "TAP TO JUMP";
    button.style.width = ui::UiSize::fill();
    button.style.height = ui::UiSize::fixed(40.0F);
    button.style.padding = 8.0F;
    button.style.background = {0.12F, 0.38F, 0.72F, 1.0F};
    button.style.foreground = {1.0F, 1.0F, 1.0F, 1.0F};
    button.style.accent = {0.25F, 0.75F, 1.0F, 1.0F};
    const auto button_result = engine.ui().create_node(
        demo.status_panel, button);
    if (!button_result) {
        log_result(logger, LogLevel::error, "ui",
                   "Could not create the Android demo touch button");
        return false;
    }
    demo.action_button = button_result.handle;
    demo.scene_created = true;

    logger.write(LogLevel::info, "demo.scene",
                 "Created combined 2D/3D physics scene and retained UI panel");
    return true;
}

bool upload_demo_gpu_resources(
    dream::runtime::EngineRuntime& engine,
    DemoState& demo,
    Logger& logger) {
    using namespace dream;

    constexpr std::array<render::Vertex3D, 3> vertices{{
        {{-0.8F, -0.6F, 0.0F}, {0.0F, 0.0F, 1.0F}, {0.0F, 1.0F}},
        {{ 0.8F, -0.6F, 0.0F}, {0.0F, 0.0F, 1.0F}, {1.0F, 1.0F}},
        {{ 0.0F,  0.8F, 0.0F}, {0.0F, 0.0F, 1.0F}, {0.5F, 0.0F}}
    }};
    constexpr std::array<std::uint32_t, 3> indices{0U, 1U, 2U};
    const render::MeshDescription mesh{vertices, indices};
    const auto mesh_result = engine.render_device().create_mesh(1U, mesh);
    if (!mesh_result) {
        logger.write(LogLevel::error, "render.resources",
                     "Demo mesh upload failed: " + mesh_result.message);
        return false;
    }

    render::MaterialDescription material;
    material.base_color = {0.25F, 0.85F, 0.45F, 1.0F};
    const auto material_result =
        engine.render_device().create_material(1U, material);
    if (!material_result) {
        logger.write(LogLevel::error, "render.resources",
                     "Demo material creation failed: " + material_result.message);
        return false;
    }

    demo.gpu_resources_created = true;
    logger.write(LogLevel::info, "render.resources",
                 "Uploaded demo 3D mesh and material to the active GPU device");
    return true;
}

bool initialize_render_surface(
    dream::runtime::EngineRuntime& engine,
    dream::platform::WindowId window,
    bool& runtime_initialized,
    bool& renderer_ready,
    DemoState& demo,
    Logger& logger) {
    if (!runtime_initialized) {
        auto result = engine.initialize(window);
        if (!result) {
            logger.write(LogLevel::critical, "runtime.lifecycle",
                         "EngineRuntime initialization failed: " +
                         result.render.message);
            return false;
        }
        runtime_initialized = true;
        if (!create_demo_scene(engine, demo, logger)) return false;
    } else {
        const auto result = engine.render_device().initialize(window);
        if (!result) {
            logger.write(LogLevel::error, "render.device",
                         "RenderDevice recreation failed: " + result.message);
            return false;
        }
    }

    renderer_ready = engine.render_device().initialized();
    if (!renderer_ready) return false;
    demo.gpu_resources_created = false;
    return upload_demo_gpu_resources(engine, demo, logger);
}

void render_demo_frame(
    dream::runtime::EngineRuntime& engine,
    dream::platform::AndroidGameActivityBackend& platform_backend,
    dream::platform::WindowId window,
    DemoState& demo,
    double delta_seconds,
    std::span<const dream::platform::InputEvent> input_events,
    HostUiMetrics& text_metrics,
    const dream::ui::BitmapGlyphProvider5x7& glyph_provider,
    Logger& logger,
    bool renderer_ready) {
    using namespace dream;

    // Do not advance simulation or consume UI input while Android has
    // suspended the app or the native render surface is unavailable.
    if (!renderer_ready || !engine.render_device().initialized()) return;

    const auto update = engine.update(delta_seconds, input_events);
    dream::ui::UiEvent ui_event;
    while (engine.ui().pop_event(ui_event)) {
        if (ui_event.type == dream::ui::UiEventType::FocusChanged) {
            if (engine.ui().is_text_field(ui_event.target)) {
                if (!platform_backend.show_soft_keyboard(
                        engine.ui().text_value(ui_event.target))) {
                    logger.write(LogLevel::warning, "input.ime",
                                 "Could not open the Android soft keyboard");
                }
            } else {
                (void)platform_backend.hide_soft_keyboard();
            }
        } else if (ui_event.type == dream::ui::UiEventType::TextChanged &&
                   ui_event.target == demo.player_name_field) {
            logger.write(LogLevel::debug, "input.ime",
                "Player name field updated; bytes=" +
                std::to_string(engine.ui().text_value(ui_event.target).size()));
        } else if (ui_event.type == dream::ui::UiEventType::Clicked &&
                   ui_event.target == demo.action_button) {
            const auto player_state =
                engine.physics2d().body_state(demo.player_2d);
            if (player_state && engine.physics2d().apply_impulse(
                    demo.player_2d, {0.0F, 5.0F}, player_state->position)) {
                logger.write(LogLevel::info, "demo.input",
                             "Touch button applied a jump impulse to the 2D player");
            } else {
                logger.write(LogLevel::warning, "demo.input",
                             "Touch button was pressed but the jump impulse was rejected");
            }
        }
    }
    if (!update) {
        logger.write(LogLevel::error, "runtime.update",
            "Engine update reported failure; error=" +
            std::to_string(static_cast<unsigned>(update.error)) +
            ", physics2d=" +
            std::to_string(static_cast<unsigned>(update.physics_error)) +
            ", physics3d=" +
            std::to_string(static_cast<unsigned>(update.physics3d_error)));
    }
    platform::WindowMetrics metrics{};
    if (!platform_backend.get_metrics(window, metrics) ||
        metrics.pixel_width == 0U || metrics.pixel_height == 0U ||
        metrics.logical_width == 0U || metrics.logical_height == 0U) {
        logger.write(LogLevel::warning, "platform.window",
                     "Skipping frame because the native window metrics are unavailable");
        return;
    }

    world::Camera2D camera_2d;
    camera_2d.position = {0.0F, 0.0F};
    camera_2d.orthographic_height = 12.0F;
    world::CameraMatrices2D camera_2d_matrices;
    if (world::build_camera_matrices(
            camera_2d, metrics.pixel_width, metrics.pixel_height,
            camera_2d_matrices) != world::CameraBuildError::None) {
        logger.write(LogLevel::error, "render.camera",
                     "Could not build the 2D camera matrices");
        return;
    }

    world::Camera3D camera_3d;
    camera_3d.position = {0.0F, 2.0F, 7.0F};
    world::CameraMatrices3D camera_3d_matrices;
    if (world::build_camera_matrices(
            camera_3d, metrics.pixel_width, metrics.pixel_height,
            camera_3d_matrices) != world::CameraBuildError::None) {
        logger.write(LogLevel::error, "render.camera",
                     "Could not build the 3D camera matrices");
        return;
    }

    // Android pointer events are already converted to logical pixels by the
    // GameActivity adapter. Layout the retained UI in that same coordinate
    // space; camera projection continues to use physical framebuffer pixels.
    const ui::UiRect viewport{
        0.0F, 0.0F,
        static_cast<float>(metrics.logical_width),
        static_cast<float>(metrics.logical_height)
    };
    if (demo.ui_width != metrics.logical_width ||
        demo.ui_height != metrics.logical_height) {
        const auto layout_error = engine.ui().layout(viewport, text_metrics);
        if (layout_error != ui::UiError::None) {
            logger.write(LogLevel::error, "ui.layout",
                         "Retained UI layout failed; ui_error=" +
                         std::to_string(static_cast<unsigned>(layout_error)));
            return;
        }
        demo.ui_width = metrics.logical_width;
        demo.ui_height = metrics.logical_height;
    }

    const auto begin = engine.begin_render_frame(
        {0.025F, 0.04F, 0.075F, 1.0F});
    if (!begin) return;

    if (engine.frame_builder().set_camera_2d(camera_2d_matrices) !=
            render::FrameBuilderError::None ||
        engine.frame_builder().set_camera_3d(camera_3d_matrices) !=
            render::FrameBuilderError::None) {
        logger.write(LogLevel::error, "render.frame",
                     "Could not attach both 2D and 3D cameras to the frame");
        return;
    }

    const auto player = engine.physics2d().body_state(demo.player_2d);
    if (player) {
        world::Transform2D transform;
        transform.position = {player->position.x, player->position.y};
        transform.rotation_radians = player->rotation_radians;
        transform.scale = {0.8F, 0.8F};
        render::SpriteQuad2D sprite;
        sprite.transform = world::transform_matrix(transform);
        sprite.tint = {0.2F, 0.75F, 1.0F, 1.0F};
        sprite.layer = 0;
        if (engine.frame_builder().add_quad(sprite) !=
            render::FrameBuilderError::None) {
            logger.write(LogLevel::error, "render.2d",
                         "Failed to add the demo player sprite to the frame");
            return;
        }
    }

    const auto ball = engine.physics3d().body_state(demo.ball_3d);
    if (ball && demo.gpu_resources_created) {
        world::Transform3D transform;
        transform.position = {ball->position.x, ball->position.y, ball->position.z};
        transform.rotation = {
            ball->rotation.x, ball->rotation.y,
            ball->rotation.z, ball->rotation.w
        };
        transform.scale = {0.85F, 0.85F, 0.85F};
        render::DrawCommand3D draw;
        draw.mesh_id = 1U;
        draw.material_id = 1U;
        draw.model = world::transform_matrix(transform);
        if (engine.frame_builder().add_mesh(draw) !=
            render::FrameBuilderError::None) {
            logger.write(LogLevel::error, "render.3d",
                         "Failed to add the demo mesh to the frame");
            return;
        }
    }

    const auto ui_result = engine.enqueue_ui_draws(
        camera_2d, viewport, &glyph_provider);
    if (ui_result != ui::UiRenderError::None) return;
    const auto rendered = engine.render_frame();
    if (!rendered) {
        logger.write(LogLevel::error, "render.frame",
                     "Demo frame submission failed: " + rendered.render.message);
    }
}

} // namespace

extern "C" void android_main(android_app* app) {
    __android_log_write(ANDROID_LOG_INFO, "DreamEngineBoot",
                        "android_main entered");
    if (app == nullptr) {
        __android_log_write(ANDROID_LOG_ERROR, "DreamEngineBoot",
                            "android_main received null android_app");
        return;
    }
    if (app->activity == nullptr) {
        __android_log_write(ANDROID_LOG_ERROR, "DreamEngineBoot",
                            "android_main received null activity");
        return;
    }
    __android_log_write(ANDROID_LOG_INFO, "DreamEngineBoot",
                        "android_app and activity are valid");

    Logger logger;
    std::vector<Logger::Sink> sinks;
    sinks.emplace_back(dream::core::make_android_logcat_sink("DreamEngine"));
    std::string log_path;
    std::string_view log_storage_scope = "unavailable";
    const char* log_directory = app->activity->externalDataPath;
    if (log_directory != nullptr && log_directory[0] != '\0') {
        log_storage_scope = "app-specific external files";
    } else {
        log_directory = app->activity->internalDataPath;
        if (log_directory != nullptr && log_directory[0] != '\0')
            log_storage_scope = "private internal files (fallback)";
    }
    if (log_directory != nullptr && log_directory[0] != '\0') {
        log_path = std::string(log_directory) + "/logs/engine.log";
        sinks.emplace_back(dream::core::make_file_log_sink(
            log_path, dream::core::FileLogSinkOptions{8U * 1024U * 1024U, 3U, true}));
    }
    logger.set_sink(dream::core::make_combined_log_sink(std::move(sinks)));
    logger.set_minimum_level(LogLevel::debug);
    logger.write(LogLevel::info, "platform.android",
                 "Dream Engine Android host entry; log_scope=" +
                 std::string(log_storage_scope) + "; log_file=" +
                 (log_path.empty() ? std::string("unavailable (Logcat only)") : log_path));

    dream::platform::AndroidAssetFileSystem apk_assets(app->activity->assetManager);
    if (!apk_assets.valid()) {
        logger.write(LogLevel::critical, "assets.apk",
                     "Android AssetManager is not available");
        return;
    }

    dream::render::BgfxClearBackend render_backend;
    std::string shader_error;
    if (!dream::render::load_bgfx_2d_shader_binaries(
            render_backend, apk_assets,
            dream::render::Bgfx2DShaderFormat::OpenGLES,
            "shaders/vs_sprite_essl300.bin",
            "shaders/fs_sprite_essl300.bin",
            shader_error)) {
        logger.write(LogLevel::critical, "render.shader",
                     "Could not load GLES3 sprite shaders: " + shader_error);
        return;
    }
    if (!dream::render::load_bgfx_3d_shader_binaries(
            render_backend, apk_assets,
            dream::render::Bgfx2DShaderFormat::OpenGLES,
            "shaders/vs_mesh_essl300.bin",
            "shaders/fs_mesh_essl300.bin",
            shader_error)) {
        logger.write(LogLevel::critical, "render.shader",
                     "Could not load GLES3 mesh shaders: " + shader_error);
        return;
    }
    __android_log_write(ANDROID_LOG_INFO, "DreamEngineBoot",
                        "APK assets and GLES shader binaries loaded");
    logger.write(LogLevel::info, "render.shader",
                 "Loaded pinned GLES 3.0 sprite and mesh shader binaries from APK assets");

    dream::platform::AndroidGameActivityBackend backend(app, 16);
    dream::runtime::EngineRuntimeConfig runtime_config;
    runtime_config.scene_name = "AndroidDemo";
    runtime_config.logger = &logger;
    dream::runtime::EngineRuntime engine(backend, render_backend, runtime_config);

    dream::audio::AndroidAAudioOutput audio_output;
    bool audio_started = false;
    bool runtime_initialized = false;
    bool renderer_ready = false;
    bool application_suspended = false;
    dream::platform::WindowId window{};
    dream::platform::Application* application = nullptr;
    DemoState demo;
    HostUiMetrics text_metrics;
    dream::ui::BitmapGlyphProvider5x7 bitmap_font;
    std::vector<dream::platform::InputEvent> input_events;
    input_events.reserve(128U);

    const auto start_audio = [&]() {
        if (audio_started) return;
        std::string error;
        if (audio_output.start(
                dream::audio::AudioOutputConfig{},
                &dream::audio::AudioMixer::render_callback,
                &engine.audio(), error)) {
            audio_started = true;
            logger.write(LogLevel::info, "audio.aaudio",
                         "AAudio output started with the engine software mixer");
        } else {
            logger.write(LogLevel::warning, "audio.aaudio",
                         "AAudio output could not start: " + error);
        }
    };

    dream::platform::ApplicationCallbacks callbacks;
    callbacks.on_initialize = [&]() {
        __android_log_write(ANDROID_LOG_INFO, "DreamEngineBoot",
                            "Application on_initialize callback entered");
        dream::platform::WindowConfig config;
        config.title = "Dream Engine";
        config.width = 1280;
        config.height = 720;
        std::string error;
        if (!backend.create_window(config, window, error)) {
            logger.write(LogLevel::critical, "platform.window",
                         "GameActivity window creation failed: " + error);
            return false;
        }
        logger.write(LogLevel::info, "platform.window",
                     "GameActivity window handle created; waiting for native surface");
        return true;
    };

    callbacks.on_update = [&](double delta_seconds) {
        dream::platform::WindowEvent window_event;
        while (backend.poll_window_event(window_event)) {
            switch (window_event.type) {
            case dream::platform::WindowEventType::SurfaceCreated:
                __android_log_write(ANDROID_LOG_INFO, "DreamEngineBoot",
                                    "GameActivity native surface created");
                logger.write(LogLevel::info, "platform.surface",
                             "Native Android render surface created");
                __android_log_write(ANDROID_LOG_INFO, "DreamEngineBoot",
                                    "Starting EngineRuntime/render surface initialization");
                if (!initialize_render_surface(
                        engine, window, runtime_initialized, renderer_ready,
                        demo, logger)) {
                    logger.write(LogLevel::critical, "runtime.lifecycle",
                                 "Could not initialize/recreate the Android render surface");
                    if (application != nullptr) application->request_stop();
                } else {
                    __android_log_write(ANDROID_LOG_INFO, "DreamEngineBoot",
                                        "EngineRuntime and renderer initialized");
                    start_audio();
                }
                break;
            case dream::platform::WindowEventType::SurfaceDestroyed:
                logger.write(LogLevel::warning, "platform.surface",
                             "Native Android render surface destroyed; releasing GPU resources");
                if (engine.render_device().initialized())
                    engine.render_device().shutdown();
                renderer_ready = false;
                demo.gpu_resources_created = false;
                break;
            case dream::platform::WindowEventType::Resized:
                logger.write(LogLevel::debug, "platform.surface",
                             "Android native surface metrics changed");
                break;
            case dream::platform::WindowEventType::FocusGained:
                logger.write(LogLevel::debug, "platform.lifecycle",
                             "Android host gained focus");
                break;
            case dream::platform::WindowEventType::FocusLost:
                logger.write(LogLevel::debug, "platform.lifecycle",
                             "Android host lost focus");
                break;
            case dream::platform::WindowEventType::Suspended:
                application_suspended = true;
                if (audio_output.running()) audio_output.stop();
                audio_started = false;
                logger.write(LogLevel::info, "platform.lifecycle",
                             "Android application suspended; audio output stopped");
                break;
            case dream::platform::WindowEventType::Resumed:
                application_suspended = false;
                logger.write(LogLevel::info, "platform.lifecycle",
                             "Android application resumed");
                if (renderer_ready) start_audio();
                break;
            case dream::platform::WindowEventType::MemoryLow:
                logger.write(LogLevel::warning, "platform.memory",
                             "Android reported low memory");
                break;
            case dream::platform::WindowEventType::CloseRequested:
                logger.write(LogLevel::info, "platform.lifecycle",
                             "Android host requested shutdown");
                if (application != nullptr) application->request_stop();
                break;
            case dream::platform::WindowEventType::Minimized:
            case dream::platform::WindowEventType::Restored:
                break;
            }
        }

        input_events.clear();
        dream::platform::InputEvent input_event;
        while (backend.poll_input_event(input_event))
            input_events.push_back(input_event);

        if (runtime_initialized) {
            render_demo_frame(
                engine, backend, window, demo, delta_seconds,
                input_events, text_metrics, bitmap_font, logger,
                renderer_ready && !application_suspended);
        }
        if (audio_started && !audio_output.running()) {
            logger.write(LogLevel::error, "audio.aaudio",
                         "AAudio stream stopped unexpectedly; a device error was recorded");
            audio_started = false;
        }
    };

    callbacks.on_shutdown = [&]() {
        logger.write(LogLevel::info, "platform.lifecycle",
                     "Android host shutdown requested");
        audio_output.stop();
        audio_started = false;
        engine.shutdown();
        if (window) backend.destroy_window(window);
        window = {};
    };

    dream::platform::Application host(
        backend,
        dream::platform::ApplicationConfig{
            .name = "Dream Engine Android Runtime",
            .max_frame_delta_seconds = 0.25
        },
        std::move(callbacks));
    application = &host;
    __android_log_write(ANDROID_LOG_INFO, "DreamEngineBoot",
                        "Entering native Application::run loop");
    if (!host.run()) {
        logger.write(LogLevel::critical, "platform.application",
                     "Application loop failed: " + host.error());
    } else {
        logger.write(LogLevel::info, "platform.application",
                     "Application loop stopped cleanly");
    }
}
#endif
