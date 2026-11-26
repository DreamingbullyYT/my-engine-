#pragma once

#include <dream/core/fixed_step_clock.hpp>
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
    render::FrameBuilderLimits frame_builder{};
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

    [[nodiscard]] explicit operator bool() const noexcept {
        return error == RuntimeError::None &&
               static_cast<bool>(fixed_step) &&
               physics_error == physics2d::PhysicsStepError::None;
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
          simulation_clock_(config_.fixed_step),
          frame_builder_(config_.frame_builder),
          render_device_(surfaces, render_backend) {
        configuration_valid_ =
            simulation_clock_.valid() &&
            physics_.valid() &&
            config_.fixed_step.fixed_delta_seconds <=
                static_cast<double>(config_.physics2d.maximum_delta_seconds) &&
            config_.frame_builder.maximum_quads_2d <=
                std::numeric_limits<std::uint32_t>::max() / 4U &&
            config_.frame_builder.maximum_quads_2d <=
                std::numeric_limits<std::uint32_t>::max() / 6U;

        const auto physics_system = scene_.systems().add_system(
            "DreamEngine.PhysicsWorld2D",
            world::SystemPhase::Update,
            std::numeric_limits<std::int32_t>::min(),
            [this](world::World&, const world::SystemContext& context) {
                const auto report = physics_.update(context.delta_seconds);
                last_physics_error_ = report.error;
                saturating_add(frame_physics_contacts_, report.contact_count);
                saturating_add(frame_physics_events_, report.events.size());
            });

        if (!physics_system) configuration_valid_ = false;
        physics_system_ = physics_system.handle;
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
        if (state_ == RuntimeState::Initialized)
            return {RuntimeError::AlreadyInitialized, {}};
        if (state_ == RuntimeState::Stopped || state_ == RuntimeState::Failed)
            return {RuntimeError::NotInitialized, {}};
        if (!configuration_valid_)
            return {RuntimeError::InvalidConfiguration, {}};

        auto render_result = render_device_.initialize(window);
        if (!render_result) {
            state_ = RuntimeState::Failed;
            return {RuntimeError::RenderFailure, std::move(render_result)};
        }

        state_ = RuntimeState::Initialized;
        return {};
    }

    [[nodiscard]] RuntimeUpdateResult update(
        double frame_delta_seconds,
        std::span<const platform::InputEvent> input_events = {}) {
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
        last_physics_error_ = physics2d::PhysicsStepError::None;

        action_map_.begin_frame();
        for (const auto& event : input_events) {
            if (action_map_.process_event(event))
                saturating_add(result.input_events_handled, 1U);
        }

        try {
            result.fixed_step = simulation_clock_.advance(
                frame_delta_seconds,
                [&](double fixed_delta, std::uint64_t) {
                    const auto report = scene_.update(
                        static_cast<float>(fixed_delta));
                    saturating_add(result.system_failures, report.failures.size());
                    if (report.error != world::SystemUpdateError::None)
                        saturating_add(result.system_failures, 1U);
                });
        } catch (...) {
            result.error = RuntimeError::SimulationCallbackFailure;
            result.physics_contacts = frame_physics_contacts_;
            result.physics_contact_events = frame_physics_events_;
            result.physics_error = last_physics_error_;
            return result;
        }

        if (!result.fixed_step)
            result.error = RuntimeError::SimulationClockFailure;

        result.physics_contacts = frame_physics_contacts_;
        result.physics_contact_events = frame_physics_events_;
        result.physics_error = last_physics_error_;
        return result;
    }

    [[nodiscard]] RuntimeRenderResult begin_render_frame(
        render::Color clear_color = {},
        bool clear_color_buffer = true,
        bool clear_depth_buffer = true) {
        if (state_ != RuntimeState::Initialized)
            return {RuntimeError::NotInitialized, {}, {}};

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

    [[nodiscard]] RuntimeRenderResult render_frame() {
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

    void shutdown() noexcept {
        if (state_ == RuntimeState::Stopped) return;
        render_device_.shutdown();
        simulation_clock_.reset();
        frame_builder_.reset();
        action_map_.reset_all();
        state_ = RuntimeState::Stopped;
    }

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

private:
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
    core::FixedStepClock simulation_clock_;
    input::ActionMap action_map_;
    render::RenderFrameBuilder frame_builder_;
    render::RenderDevice render_device_;
    world::SystemHandle physics_system_{};
    bool configuration_valid_{false};
    RuntimeState state_{RuntimeState::Created};
    double last_frame_delta_seconds_{0.0};
    std::uint64_t render_frame_index_{0};
    std::size_t frame_physics_contacts_{0};
    std::size_t frame_physics_events_{0};
    physics2d::PhysicsStepError last_physics_error_{
        physics2d::PhysicsStepError::None};
};

} // namespace dream::runtime
