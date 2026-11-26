#pragma once

#include <dream/core/fixed_step_clock.hpp>
#include <dream/core/log.hpp>
#include <dream/animation/animation_runtime.hpp>
#include <dream/assets/builtin_importers.hpp>
#include <dream/audio/audio_mixer.hpp>
#include <dream/physics3d/physics_world.hpp>
#include <dream/ui/ui_render_adapter.hpp>
#include <dream/dmod/native_mod_host.hpp>
#include <dream/dmod/native_mod_manager.hpp>
#include <dream/input/action_map.hpp>
#include <dream/physics2d/physics_world.hpp>
#include <dream/platform/surface.hpp>
#include <dream/render/render_backend.hpp>
#include <dream/render/render_frame_builder.hpp>
#include <dream/world/scene.hpp>

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <stdexcept>
#include <string>
#include <utility>

namespace dream::runtime {

struct EngineRuntimeConfig final {
    std::string scene_name{"Main"};
    core::FixedStepConfig fixed_step{};
    physics2d::PhysicsSettings2D physics2d{};
    physics3d::PhysicsSettings3D physics3d{};
    render::FrameBuilderLimits frame_builder{};
    core::Logger* logger{nullptr};
};

enum class RuntimeState : std::uint8_t {
    Created,
    Initialized,
    Stopped,
    Failed
};

enum class RuntimeError : std::uint8_t {
    None,
    InvalidConfiguration,
    AlreadyInitialized,
    NotInitialized,
    InvalidFrameDelta,
    SimulationClockFailure,
    SimulationCallbackFailure,
    FrameBuilderFailure,
    RenderFailure
};

struct RuntimeInitResult final {
    RuntimeError error{RuntimeError::None};
    render::RenderResult render{};

    [[nodiscard]] explicit operator bool() const noexcept {
        return error == RuntimeError::None && static_cast<bool>(render);
    }
};

struct RuntimeUpdateResult final {
    RuntimeError error{RuntimeError::None};
    core::FixedStepAdvanceResult fixed_step{};
    std::size_t input_events_handled{0};
    std::size_t system_failures{0};
    std::size_t physics_contacts{0};
    std::size_t physics_contact_events{0};
    physics2d::PhysicsStepError physics_error{physics2d::PhysicsStepError::None};
    physics3d::PhysicsStepError3D physics3d_error{physics3d::PhysicsStepError3D::None};
    std::size_t physics3d_contacts{0};
    std::size_t physics3d_contact_events{0};

    [[nodiscard]] explicit operator bool() const noexcept {
        return error == RuntimeError::None &&
               static_cast<bool>(fixed_step) &&
               physics_error == physics2d::PhysicsStepError::None &&
               physics3d_error == physics3d::PhysicsStepError3D::None;
    }
};

struct RuntimeRenderResult final {
    RuntimeError error{RuntimeError::None};
    render::FrameBuilderError frame_error{render::FrameBuilderError::None};
    render::RenderResult render{};

    [[nodiscard]] explicit operator bool() const noexcept {
        return error == RuntimeError::None &&
               frame_error == render::FrameBuilderError::None &&
               static_cast<bool>(render);
    }
};

class EngineRuntime;

namespace hook_points {
inline constexpr char runtime_update[] = "dream.engine.runtime.update";
inline constexpr char physics2d_step[] = "dream.engine.physics2d.step";
inline constexpr char physics3d_step[] = "dream.engine.physics3d.step";
inline constexpr char scene_system[] = "dream.engine.scene.system";
inline constexpr char begin_render_frame[] = "dream.engine.runtime.begin_render_frame";
inline constexpr char render_frame[] = "dream.engine.runtime.render_frame";
inline constexpr char animation_update[] = "dream.engine.animation.update";
inline constexpr char asset_import[] = "dream.engine.assets.import";
inline constexpr char ui_draw[] = "dream.engine.ui.draw";
inline constexpr char audio_mix[] = "dream.engine.audio.mix";
} // namespace hook_points

// These contexts live only for the duration of a hook dispatch. Mods may mutate
// the fields and may replace the whole operation by omitting call_next.
struct RuntimeUpdateHookDataV1 final {
    EngineRuntime* runtime{};
    double frame_delta_seconds{};
    std::span<const platform::InputEvent> input_events{};
    RuntimeUpdateResult result{};
};

struct RuntimePhysics2DStepHookDataV1 final {
    EngineRuntime* runtime{};
    float delta_seconds{};
    physics2d::PhysicsStepReport2D report{};
};
struct RuntimePhysics3DStepHookDataV1 final {
    EngineRuntime* runtime{};
    float delta_seconds{};
    physics3d::PhysicsStepReport3D report{};
};
struct RuntimeSystemHookDataV1 final {
    EngineRuntime* runtime{};
    world::SystemExecutionInfo system{};
    // Pointers refer to the current scheduler invocation and are valid only
    // while this native hook dispatch is executing.
    world::World** world{};
    world::SystemContext* context{};
    world::SystemExecutionThunk original{};
    void* original_context{};
};

struct RuntimeBeginRenderFrameHookDataV1 final {
    EngineRuntime* runtime{};
    render::Color clear_color{};
    bool clear_color_buffer{true};
    bool clear_depth_buffer{true};
    RuntimeRenderResult result{};
};

struct RuntimeRenderFrameHookDataV1 final {
    EngineRuntime* runtime{};
    RuntimeRenderResult result{};
};

struct RuntimeAnimationUpdateHookDataV1 final {
    EngineRuntime* runtime{};
    world::World* world{};
    double delta_seconds{};
    animation::AnimationUpdateReport report{};
};

struct RuntimeAssetImportHookDataV1 final {
    EngineRuntime* runtime{};
    std::string_view source_path{};
    std::span<const std::byte> source_bytes{};
    std::string_view options{};
    std::string importer_id;
    assets::AssetImporter importer;
    assets::AssetImporterOutput output{};
    assets::AssetImporterRegistryError registry_error{
        assets::AssetImporterRegistryError::None};
};

struct RuntimeUiDrawHookDataV1 final {
    EngineRuntime* runtime{};
    ui::UiRenderAdapter* adapter{};
    ui::UiRenderError result{ui::UiRenderError::None};
};

struct RuntimeAudioMixHookDataV1 final {
    EngineRuntime* runtime{};
    std::span<float> output{};
    std::size_t frame_count{};
    std::uint32_t sample_rate{};
    audio::AudioError result{audio::AudioError::None};
};

// Coordinates one application-facing engine loop. Input is collected once per
// rendered frame; the Scene scheduler and built-in PhysicsWorld2D system run
// at the fixed simulation rate. The physics system runs at the lowest Update
// priority, after normal gameplay Update systems and before PostUpdate.
//
// The runtime does not own platform window creation or event polling. The host
// supplies the WindowId, native surface system and input-event span. A platform
// application loop can call update(), begin_render_frame(), build draw commands
// and render_frame() once per rendered frame.
//
// ActionMap edge flags are latched for the rendered frame. If a render frame
// produces multiple fixed simulation ticks, every tick observes the same input
// snapshot; gameplay systems should consume one-shot actions deliberately.
//
// User-defined physics/entity synchronization is not implicit. Gameplay
// systems can read PhysicsWorld2D body state and update ECS components in
// PostUpdate (or use their own registered systems).
class EngineRuntime final {
public:
    EngineRuntime(
        platform::ISurfaceSystem& surfaces,
        render::IRenderBackend& render_backend,
        EngineRuntimeConfig config = {})
        : config_(std::move(config)),
          scene_(config_.scene_name),
          physics_(config_.physics2d),
          physics3d_(config_.physics3d),
          simulation_clock_(config_.fixed_step),
          frame_builder_(config_.frame_builder),
          render_device_(surfaces, render_backend),
          mod_host_(),
          mod_manager_(mod_host_.api()) {
        bool interfaces_valid = true;
        interfaces_valid = mod_host_.expose("dream.engine.runtime", 1, *this) && interfaces_valid;
        interfaces_valid = mod_host_.expose("dream.engine.scene", 1, scene_) && interfaces_valid;
        interfaces_valid = mod_host_.expose("dream.engine.world", 1, scene_.world()) && interfaces_valid;
        interfaces_valid = mod_host_.expose("dream.engine.physics2d", 1, physics_) && interfaces_valid;
        interfaces_valid = mod_host_.expose("dream.engine.physics3d", 1, physics3d_) && interfaces_valid;
        interfaces_valid = mod_host_.expose("dream.engine.animation", 1, animation_runtime_) && interfaces_valid;
        interfaces_valid = mod_host_.expose("dream.engine.audio", 1, audio_mixer_) && interfaces_valid;
        interfaces_valid = mod_host_.expose("dream.engine.ui", 1, ui_runtime_) && interfaces_valid;
        interfaces_valid = mod_host_.expose("dream.engine.assets.importers", 1, asset_importers_) && interfaces_valid;
        interfaces_valid = mod_host_.expose("dream.engine.input.actions", 1, action_map_) && interfaces_valid;
        interfaces_valid = mod_host_.expose("dream.engine.clock.fixed", 1, simulation_clock_) && interfaces_valid;
        interfaces_valid = mod_host_.expose("dream.engine.render.frame_builder", 1, frame_builder_) && interfaces_valid;
        interfaces_valid = mod_host_.expose("dream.engine.render.device", 1, render_device_) && interfaces_valid;
        interfaces_valid = mod_host_.expose("dream.engine.mods.manager", 1, mod_manager_) && interfaces_valid;
        interfaces_valid = mod_host_.expose("dream.engine.mods.hooks", 1, mod_manager_.hook_registry()) && interfaces_valid;

        scene_.systems().set_execution_dispatcher(
            this, &EngineRuntime::dispatch_scene_system);

        configuration_valid_ =
            interfaces_valid &&
            simulation_clock_.valid() &&
            physics_.valid() &&
            physics3d_.valid() &&
            config_.fixed_step.fixed_delta_seconds <=
                static_cast<double>(config_.physics2d.maximum_delta_seconds) &&
            config_.fixed_step.fixed_delta_seconds <=
                static_cast<double>(config_.physics3d.maximum_delta_seconds) &&
            config_.frame_builder.maximum_quads_2d <=
                std::numeric_limits<std::uint32_t>::max() / 4U &&
            config_.frame_builder.maximum_quads_2d <=
                std::numeric_limits<std::uint32_t>::max() / 6U;

        configuration_valid_ = configuration_valid_ &&
            assets::register_builtin_importers(asset_importers_) ==
                assets::AssetImporterRegistryError::None;

        log_message(
            configuration_valid_ ? core::LogLevel::info : core::LogLevel::error,
            "runtime.lifecycle",
            configuration_valid_
                ? "EngineRuntime constructed; core services and built-in importers are ready"
                : "EngineRuntime configuration or service registration validation failed");

        const auto physics_system = scene_.systems().add_system(
            "DreamEngine.PhysicsWorld2D",
            world::SystemPhase::Update,
            std::numeric_limits<std::int32_t>::min(),
            [this](world::World&, const world::SystemContext& context) {
                RuntimePhysics2DStepHookDataV1 hook_data{
                    this, context.delta_seconds, {}
                };
                const auto dispatched = mod_manager_.dispatch_hooks(
                    hook_points::physics2d_step,
                    &hook_data,
                    [](void* opaque) noexcept {
                        auto& call =
                            *static_cast<RuntimePhysics2DStepHookDataV1*>(opaque);
                        try {
                            call.report = call.runtime->physics_.update(call.delta_seconds);
                        } catch (...) {
                            call.report.error = physics2d::PhysicsStepError::InvalidBodyState;
                        }
                    });
                if (!dispatched.success) {
                    last_physics_error_ = physics2d::PhysicsStepError::InvalidBodyState;
                    return;
                }
                last_physics_error_ = hook_data.report.error;
                saturating_add(frame_physics_contacts_, hook_data.report.contact_count);
                saturating_add(frame_physics_events_, hook_data.report.events.size());
            });

        const auto animation_system = scene_.systems().add_system(
            "DreamEngine.AnimationRuntime",
            world::SystemPhase::PreUpdate,
            std::numeric_limits<std::int32_t>::min(),
            [this](world::World& world, const world::SystemContext& context) {
                RuntimeAnimationUpdateHookDataV1 hook_data{
                    this, &world, static_cast<double>(context.delta_seconds), {}
                };
                (void)mod_manager_.dispatch_hooks(
                    hook_points::animation_update,
                    &hook_data,
                    [](void* opaque) noexcept {
                        auto& call =
                            *static_cast<RuntimeAnimationUpdateHookDataV1*>(opaque);
                        try {
                            call.report = call.runtime->animation_runtime_.update(
                                *call.world, call.delta_seconds);
                        } catch (...) {
                            call.report.error = animation::AnimationError::InvalidDelta;
                        }
                    });
                last_animation_report_ = hook_data.report;
            });

        const auto physics3d_system = scene_.systems().add_system(
            "DreamEngine.PhysicsWorld3D",
            world::SystemPhase::Update,
            std::numeric_limits<std::int32_t>::min(),
            [this](world::World&, const world::SystemContext& context) {
                RuntimePhysics3DStepHookDataV1 hook_data{
                    this, context.delta_seconds, {}
                };
                const auto dispatched = mod_manager_.dispatch_hooks(
                    hook_points::physics3d_step,
                    &hook_data,
                    [](void* opaque) noexcept {
                        auto& call =
                            *static_cast<RuntimePhysics3DStepHookDataV1*>(opaque);
                        try {
                            call.report = call.runtime->physics3d_.update(
                                call.delta_seconds);
                        } catch (...) {
                            call.report.error =
                                physics3d::PhysicsStepError3D::InvalidBodyState;
                        }
                    });
                if (!dispatched.success) {
                    last_physics3d_error_ =
                        physics3d::PhysicsStepError3D::InvalidBodyState;
                    return;
                }
                last_physics3d_error_ = hook_data.report.error;
                saturating_add(frame_physics3d_contacts_,
                    hook_data.report.contact_count);
                saturating_add(frame_physics3d_events_,
                    hook_data.report.events.size());
            });

        if (!physics_system || !animation_system || !physics3d_system)
            configuration_valid_ = false;
        physics_system_ = physics_system.handle;
        animation_system_ = animation_system.handle;
        physics3d_system_ = physics3d_system.handle;
    }

    EngineRuntime(const EngineRuntime&) = delete;
    EngineRuntime& operator=(const EngineRuntime&) = delete;
    EngineRuntime(EngineRuntime&&) = delete;
    EngineRuntime& operator=(EngineRuntime&&) = delete;

    ~EngineRuntime() { shutdown(); }

    [[nodiscard]] RuntimeState state() const noexcept { return state_; }
    [[nodiscard]] bool configuration_valid() const noexcept {
        return configuration_valid_;
    }

    [[nodiscard]] RuntimeInitResult initialize(platform::WindowId window) {
        if (state_ == RuntimeState::Initialized) {
            log_message(core::LogLevel::warning, "runtime.lifecycle",
                        "initialize rejected: EngineRuntime is already initialized");
            return {RuntimeError::AlreadyInitialized, {}};
        }
        if (state_ == RuntimeState::Stopped || state_ == RuntimeState::Failed) {
            log_message(core::LogLevel::error, "runtime.lifecycle",
                        "initialize rejected: stopped/failed runtime cannot be restarted");
            return {RuntimeError::NotInitialized, {}};
        }
        if (!configuration_valid_) {
            log_message(core::LogLevel::critical, "runtime.lifecycle",
                        "initialize aborted: runtime configuration is invalid");
            return {RuntimeError::InvalidConfiguration, {}};
        }

        log_message(core::LogLevel::info, "render.device",
                    "initializing RenderDevice and acquiring the native surface");
        auto render_result = render_device_.initialize(window);
        if (!render_result) {
            log_message(core::LogLevel::error, "render.device",
                        "RenderDevice initialization failed: " + render_result.message);
            state_ = RuntimeState::Failed;
            return {RuntimeError::RenderFailure, std::move(render_result)};
        }

        state_ = RuntimeState::Initialized;
        log_message(core::LogLevel::info, "runtime.lifecycle",
                    "EngineRuntime initialized successfully");
        return {};
    }

    [[nodiscard]] RuntimeUpdateResult update(
        double frame_delta_seconds,
        std::span<const platform::InputEvent> input_events = {}) {
        RuntimeUpdateHookDataV1 hook_data{
            this, frame_delta_seconds, input_events, {}
        };
        (void)mod_manager_.dispatch_hooks(
            hook_points::runtime_update,
            &hook_data,
            [](void* opaque) noexcept {
                auto& call = *static_cast<RuntimeUpdateHookDataV1*>(opaque);
                try {
                    call.result = call.runtime->update_unhooked(
                        call.frame_delta_seconds, call.input_events);
                } catch (...) {
                    call.result.error = RuntimeError::SimulationCallbackFailure;
                }
            });
        if (!hook_data.result) {
            log_message(core::LogLevel::error, "runtime.update",
                "frame update failed; runtime_error=" +
                std::to_string(static_cast<unsigned>(hook_data.result.error)) +
                ", physics2d_error=" +
                std::to_string(static_cast<unsigned>(hook_data.result.physics_error)) +
                ", physics3d_error=" +
                std::to_string(static_cast<unsigned>(hook_data.result.physics3d_error)) +
                ", system_failures=" +
                std::to_string(hook_data.result.system_failures));
        }
        return hook_data.result;
    }

    [[nodiscard]] RuntimeRenderResult begin_render_frame(
        render::Color clear_color = {},
        bool clear_color_buffer = true,
        bool clear_depth_buffer = true) {
        RuntimeBeginRenderFrameHookDataV1 hook_data{
            this, clear_color, clear_color_buffer, clear_depth_buffer, {}
        };
        (void)mod_manager_.dispatch_hooks(
            hook_points::begin_render_frame,
            &hook_data,
            [](void* opaque) noexcept {
                auto& call =
                    *static_cast<RuntimeBeginRenderFrameHookDataV1*>(opaque);
                try {
                    call.result = call.runtime->begin_render_frame_unhooked(
                        call.clear_color, call.clear_color_buffer,
                        call.clear_depth_buffer);
                } catch (...) {
                    call.result.error = RuntimeError::FrameBuilderFailure;
                }
            });
        if (!hook_data.result) {
            log_message(core::LogLevel::error, "render.frame",
                "begin_render_frame failed; runtime_error=" +
                std::to_string(static_cast<unsigned>(hook_data.result.error)) +
                ", frame_error=" +
                std::to_string(static_cast<unsigned>(hook_data.result.frame_error)) +
                ", backend_error=" +
                std::to_string(static_cast<unsigned>(hook_data.result.render.error)) +
                ", detail=" + hook_data.result.render.message);
        }
        return hook_data.result;
    }

    // Call after begin_render_frame() and before render_frame(). The supplied
    // Camera2D must be the same camera used for the frame's 2D camera matrices.
    [[nodiscard]] ui::UiRenderError enqueue_ui_draws(
        const world::Camera2D& camera, ui::UiRect viewport,
        const ui::IUiGlyphProvider* glyph_provider = nullptr,
        std::int32_t layer = 100000) {
        if (state_ != RuntimeState::Initialized) {
            log_message(core::LogLevel::warning, "ui.runtime",
                        "UI draw enqueue rejected: EngineRuntime is not initialized");
            return ui::UiRenderError::RuntimeNotInitialized;
        }
        ui::UiRenderAdapter adapter(
            frame_builder_, viewport, camera, glyph_provider, layer);
        if (!adapter.valid()) {
            log_message(core::LogLevel::error, "ui.render",
                        "UI render adapter rejected the requested viewport or draw state");
            return adapter.error();
        }
        RuntimeUiDrawHookDataV1 hook_data{this, &adapter, ui::UiRenderError::None};
        (void)mod_manager_.dispatch_hooks(
            hook_points::ui_draw,
            &hook_data,
            [](void* opaque) noexcept {
                auto& call = *static_cast<RuntimeUiDrawHookDataV1*>(opaque);
                call.runtime->ui_runtime_.draw(*call.adapter);
                call.result = call.adapter->error();
            });
        if (hook_data.result != ui::UiRenderError::None) {
            log_message(core::LogLevel::error, "ui.render",
                "UI draw failed; ui_error=" +
                std::to_string(static_cast<unsigned>(hook_data.result)));
        }
        return hook_data.result;
    }

    // Ordinary-thread mixing is hookable. Do not call this from an audio device
    // callback: real-time callbacks must use AudioMixer::mix_stereo_realtime()
    // and must never execute arbitrary native mod code.
    [[nodiscard]] audio::AudioError mix_audio(
        std::span<float> output, std::size_t frame_count,
        std::uint32_t sample_rate) {
        RuntimeAudioMixHookDataV1 hook_data{
            this, output, frame_count, sample_rate, audio::AudioError::None
        };
        (void)mod_manager_.dispatch_hooks(
            hook_points::audio_mix,
            &hook_data,
            [](void* opaque) noexcept {
                auto& call = *static_cast<RuntimeAudioMixHookDataV1*>(opaque);
                call.result = call.runtime->audio_mixer_.mix_stereo(
                    call.output, call.frame_count, call.sample_rate);
            });
        return hook_data.result;
    }

    // Runs the registered importer against an in-memory source. AssetImportPipeline
    // remains responsible for filesystem reads, output validation and hashing.
    // Mods may intercept/replace this importer invocation through asset_import.
    [[nodiscard]] RuntimeAssetImportHookDataV1 import_asset_source(
        std::string_view source_path, std::span<const std::byte> source_bytes,
        std::string_view options = {}) {
        RuntimeAssetImportHookDataV1 hook_data;
        hook_data.runtime = this;
        hook_data.source_path = source_path;
        hook_data.source_bytes = source_bytes;
        hook_data.options = options;
        if (!asset_importers_.find_for_path(
                source_path, hook_data.importer_id, hook_data.importer)) {
            hook_data.registry_error =
                assets::AssetImporterRegistryError::NotFound;
            return hook_data;
        }
        (void)mod_manager_.dispatch_hooks(
            hook_points::asset_import,
            &hook_data,
            [](void* opaque) noexcept {
                auto& call = *static_cast<RuntimeAssetImportHookDataV1*>(opaque);
                try {
                    call.output = call.importer(
                        call.source_path, call.source_bytes, call.options);
                } catch (const std::exception& exception) {
                    call.output.error = exception.what();
                } catch (...) {
                    call.output.error = "Native asset importer threw an unknown exception";
                }
            });
        return hook_data;
    }

    [[nodiscard]] const animation::AnimationUpdateReport&
    last_animation_report() const noexcept {
        return last_animation_report_;
    }

    [[nodiscard]] RuntimeRenderResult render_frame() {
        RuntimeRenderFrameHookDataV1 hook_data{this, {}};
        (void)mod_manager_.dispatch_hooks(
            hook_points::render_frame,
            &hook_data,
            [](void* opaque) noexcept {
                auto& call = *static_cast<RuntimeRenderFrameHookDataV1*>(opaque);
                try {
                    call.result = call.runtime->render_frame_unhooked();
                } catch (...) {
                    call.result.error = RuntimeError::RenderFailure;
                }
            });
        if (!hook_data.result) {
            log_message(core::LogLevel::error, "render.frame",
                "render_frame failed; runtime_error=" +
                std::to_string(static_cast<unsigned>(hook_data.result.error)) +
                ", frame_error=" +
                std::to_string(static_cast<unsigned>(hook_data.result.frame_error)) +
                ", backend_error=" +
                std::to_string(static_cast<unsigned>(hook_data.result.render.error)) +
                ", detail=" + hook_data.result.render.message);
        }
        return hook_data.result;
    }

    void shutdown() noexcept {
        if (state_ == RuntimeState::Stopped) return;
        log_message(core::LogLevel::info, "runtime.lifecycle",
                    "EngineRuntime shutdown started");
        mod_manager_.shutdown();
        render_device_.shutdown();
        simulation_clock_.reset();
        frame_builder_.reset();
        action_map_.reset_all();
        state_ = RuntimeState::Stopped;
        log_message(core::LogLevel::info, "runtime.lifecycle",
                    "EngineRuntime shutdown completed");
    }

    // Native mods can query these interfaces by ID from NativeModHostApiV1.
    // They are direct pointers to live engine objects, not sandboxed proxies.
    [[nodiscard]] dmod::NativeModHost& mod_host() noexcept { return mod_host_; }
    [[nodiscard]] dmod::NativeModManager& mods() noexcept { return mod_manager_; }

    [[nodiscard]] world::Scene& scene() noexcept { return scene_; }
    [[nodiscard]] const world::Scene& scene() const noexcept { return scene_; }

    [[nodiscard]] input::ActionMap& actions() noexcept { return action_map_; }
    [[nodiscard]] const input::ActionMap& actions() const noexcept {
        return action_map_;
    }

    [[nodiscard]] physics2d::PhysicsWorld2D& physics2d() noexcept {
        return physics_;
    }
    [[nodiscard]] const physics2d::PhysicsWorld2D& physics2d() const noexcept {
        return physics_;
    }
    [[nodiscard]] physics3d::PhysicsWorld3D& physics3d() noexcept {
        return physics3d_;
    }
    [[nodiscard]] const physics3d::PhysicsWorld3D& physics3d() const noexcept {
        return physics3d_;
    }
    [[nodiscard]] animation::AnimationRuntime& animations() noexcept {
        return animation_runtime_;
    }
    [[nodiscard]] audio::AudioMixer& audio() noexcept { return audio_mixer_; }
    [[nodiscard]] ui::UiRuntime& ui() noexcept { return ui_runtime_; }
    [[nodiscard]] assets::AssetImporterRegistry& asset_importers() noexcept {
        return asset_importers_;
    }


    [[nodiscard]] core::FixedStepClock& simulation_clock() noexcept {
        return simulation_clock_;
    }
    [[nodiscard]] const core::FixedStepClock& simulation_clock() const noexcept {
        return simulation_clock_;
    }

    [[nodiscard]] render::RenderFrameBuilder& frame_builder() noexcept {
        return frame_builder_;
    }
    [[nodiscard]] const render::RenderFrameBuilder& frame_builder() const noexcept {
        return frame_builder_;
    }

    [[nodiscard]] render::RenderDevice& render_device() noexcept {
        return render_device_;
    }
    [[nodiscard]] const render::RenderDevice& render_device() const noexcept {
        return render_device_;
    }

    [[nodiscard]] world::SystemHandle physics_system() const noexcept {
        return physics_system_;
    }
    [[nodiscard]] world::SystemHandle physics3d_system() const noexcept {
        return physics3d_system_;
    }
    [[nodiscard]] world::SystemHandle animation_system() const noexcept {
        return animation_system_;
    }

private:
    void log_message(
        core::LogLevel level,
        std::string_view category,
        std::string message,
        const std::source_location& source = std::source_location::current()) const noexcept {
        if (config_.logger == nullptr) return;
        try {
            config_.logger->write(level, category, message, source);
        } catch (...) {
            // Diagnostics must never turn a recoverable engine error into a crash.
        }
    }

    static void dispatch_scene_system(
        void* context,
        const world::SystemExecutionInfo& system,
        world::World*& world,
        world::SystemContext& system_context,
        world::SystemExecutionThunk original,
        void* original_context) noexcept {
        if (context == nullptr || original == nullptr) {
            if (original != nullptr) original(original_context);
            return;
        }
        auto& runtime = *static_cast<EngineRuntime*>(context);
        RuntimeSystemHookDataV1 hook_data{
            &runtime, system, &world, &system_context, original, original_context
        };
        (void)runtime.mod_manager_.dispatch_hooks(
            hook_points::scene_system,
            &hook_data,
            [](void* opaque) noexcept {
                if (opaque == nullptr) return;
                auto& call = *static_cast<RuntimeSystemHookDataV1*>(opaque);
                if (call.original != nullptr)
                    call.original(call.original_context);
            });
    }

    RuntimeUpdateResult update_unhooked(
        double frame_delta_seconds,
        std::span<const platform::InputEvent> input_events) {
        RuntimeUpdateResult result;
        if (state_ != RuntimeState::Initialized) {
            result.error = RuntimeError::NotInitialized;
            return result;
        }
        if (!std::isfinite(frame_delta_seconds) || frame_delta_seconds < 0.0) {
            result.error = RuntimeError::InvalidFrameDelta;
            return result;
        }

        last_frame_delta_seconds_ = frame_delta_seconds;
        frame_physics_contacts_ = 0U;
        frame_physics_events_ = 0U;
        frame_physics3d_contacts_ = 0U;
        frame_physics3d_events_ = 0U;
        last_physics_error_ = physics2d::PhysicsStepError::None;
        last_physics3d_error_ = physics3d::PhysicsStepError3D::None;

        action_map_.begin_frame();
        for (const auto& event : input_events) {
            bool handled = action_map_.process_event(event);
            switch (event.type) {
            case platform::InputEventType::PointerDown:
                handled = ui_runtime_.pointer_event({
                    ui::PointerAction::Down, event.x, event.y}) ==
                    ui::UiError::None || handled;
                break;
            case platform::InputEventType::PointerMove:
                handled = ui_runtime_.pointer_event({
                    ui::PointerAction::Move, event.x, event.y}) ==
                    ui::UiError::None || handled;
                break;
            case platform::InputEventType::PointerUp:
                handled = ui_runtime_.pointer_event({
                    ui::PointerAction::Up, event.x, event.y}) ==
                    ui::UiError::None || handled;
                break;
            case platform::InputEventType::PointerCancel:
                handled = ui_runtime_.pointer_event({
                    ui::PointerAction::Cancel, event.x, event.y}) ==
                    ui::UiError::None || handled;
                break;
            case platform::InputEventType::KeyDown:
                // USB HID Keyboard/Keypad usages: Tab, Enter, arrows and Space.
                if (event.key_code == 0x28U || event.key_code == 0x2CU) {
                    handled = ui_runtime_.activate_focused() || handled;
                } else if (event.key_code == 0x2BU ||
                           event.key_code == 0x4FU ||
                           event.key_code == 0x50U ||
                           event.key_code == 0x51U ||
                           event.key_code == 0x52U) {
                    const bool reverse = event.modifiers.shift ||
                        event.key_code == 0x50U || event.key_code == 0x52U;
                    handled = ui_runtime_.focus_next(reverse) || handled;
                }
                break;
            default:
                break;
            }
            if (handled)
                saturating_add(result.input_events_handled, std::size_t{1});
        }

        try {
            result.fixed_step = simulation_clock_.advance(
                frame_delta_seconds,
                [&](double fixed_delta, std::uint64_t) {
                    const auto report = scene_.update(
                        static_cast<float>(fixed_delta));
                    saturating_add(result.system_failures, report.failures.size());
                    if (report.error != world::SystemUpdateError::None)
                        saturating_add(result.system_failures, std::size_t{1});
                    if (!mod_manager_.update(fixed_delta))
                        saturating_add(result.system_failures, std::size_t{1});
                });
        } catch (...) {
            result.error = RuntimeError::SimulationCallbackFailure;
            result.physics_contacts = frame_physics_contacts_;
            result.physics_contact_events = frame_physics_events_;
            result.physics_error = last_physics_error_;
            result.physics3d_error = last_physics3d_error_;
            result.physics3d_contacts = frame_physics3d_contacts_;
            result.physics3d_contact_events = frame_physics3d_events_;
            return result;
        }

        if (!result.fixed_step)
            result.error = RuntimeError::SimulationClockFailure;

        result.physics_contacts = frame_physics_contacts_;
        result.physics_contact_events = frame_physics_events_;
        result.physics_error = last_physics_error_;
        result.physics3d_error = last_physics3d_error_;
        result.physics3d_contacts = frame_physics3d_contacts_;
        result.physics3d_contact_events = frame_physics3d_events_;
        return result;
    }

    RuntimeRenderResult begin_render_frame_unhooked(
        render::Color clear_color = {},
        bool clear_color_buffer = true,
        bool clear_depth_buffer = true) {
        if (state_ != RuntimeState::Initialized)
            return {RuntimeError::NotInitialized, {}, {}};

        auto surface_result = render_device_.refresh_surface();
        if (!surface_result)
            return {
                RuntimeError::RenderFailure, {},
                std::move(surface_result)
            };

        const auto extent = render_device_.surface_info().pixel_extent;
        const auto error = frame_builder_.begin_frame(
            render_frame_index_,
            last_frame_delta_seconds_,
            extent,
            clear_color,
            clear_color_buffer,
            clear_depth_buffer);
        if (error != render::FrameBuilderError::None)
            return {RuntimeError::FrameBuilderFailure, error, {}};

        if (render_frame_index_ != std::numeric_limits<std::uint64_t>::max())
            ++render_frame_index_;
        return {};
    }

    RuntimeRenderResult render_frame_unhooked() {
        if (state_ != RuntimeState::Initialized)
            return {RuntimeError::NotInitialized, {}, {}};

        const auto build_error = frame_builder_.finalize();
        if (build_error != render::FrameBuilderError::None)
            return {RuntimeError::FrameBuilderFailure, build_error, {}};

        auto render_result = render_device_.render(frame_builder_.packet());
        if (!render_result)
            return {
                RuntimeError::RenderFailure, {},
                std::move(render_result)
            };
        return {};
    }

    template <typename Value>
    static void saturating_add(Value& target, Value amount) noexcept {
        if (amount > std::numeric_limits<Value>::max() - target)
            target = std::numeric_limits<Value>::max();
        else
            target += amount;
    }

    EngineRuntimeConfig config_;
    world::Scene scene_;
    physics2d::PhysicsWorld2D physics_;
    physics3d::PhysicsWorld3D physics3d_;
    core::FixedStepClock simulation_clock_;
    animation::AnimationRuntime animation_runtime_;
    audio::AudioMixer audio_mixer_;
    ui::UiRuntime ui_runtime_;
    assets::AssetImporterRegistry asset_importers_;
    input::ActionMap action_map_;
    render::RenderFrameBuilder frame_builder_;
    render::RenderDevice render_device_;
    dmod::NativeModHost mod_host_;
    dmod::NativeModManager mod_manager_;
    world::SystemHandle physics_system_{};
    world::SystemHandle physics3d_system_{};
    world::SystemHandle animation_system_{};
    bool configuration_valid_{false};
    RuntimeState state_{RuntimeState::Created};
    double last_frame_delta_seconds_{0.0};
    std::uint64_t render_frame_index_{0};
    std::size_t frame_physics_contacts_{0};
    std::size_t frame_physics_events_{0};
    std::size_t frame_physics3d_contacts_{0};
    std::size_t frame_physics3d_events_{0};
    physics2d::PhysicsStepError last_physics_error_{
        physics2d::PhysicsStepError::None};
    physics3d::PhysicsStepError3D last_physics3d_error_{
        physics3d::PhysicsStepError3D::None};
    animation::AnimationUpdateReport last_animation_report_{};
};

} // namespace dream::runtime
