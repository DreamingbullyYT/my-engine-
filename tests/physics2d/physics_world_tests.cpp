#include <dream/physics2d/physics_world.hpp>
#include <dream/core/fixed_step_clock.hpp>

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string_view>

namespace {
using namespace dream::physics2d;

void require(bool condition, std::string_view message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

bool near(float left, float right, float epsilon = 0.02F) {
    return std::abs(left - right) <= epsilon;
}
}

int main() {
    PhysicsWorld2D invalid_world(PhysicsSettings2D{
        .gravity = {0, 0},
        .velocity_iterations = 0U
    });
    require(!invalid_world.valid(), "invalid physics settings are reported");
    require(invalid_world.update(0.01).error == PhysicsStepError::InvalidSettings,
            "invalid world refuses simulation");
    require(invalid_world.configure(PhysicsSettings2D{
                .gravity = {0, -9.81F},
                .velocity_iterations = 8U}),
            "valid settings can be configured");

    PhysicsWorld2D world(PhysicsSettings2D{
        .gravity = {0, 0},
        .velocity_iterations = 8U
    });

    BodyDescription2D invalid_mass;
    invalid_mass.type = BodyType::Dynamic;
    invalid_mass.mass = 0.0F;
    require(world.create_body(invalid_mass).error == BodyCreateError::InvalidMass,
            "dynamic body rejects zero mass");

    BodyDescription2D invalid_collider;
    invalid_collider.collider = CircleCollider2D{{}, -1.0F};
    require(world.create_body(invalid_collider).error == BodyCreateError::InvalidCollider,
            "negative-radius collider is rejected");

    BodyDescription2D invalid_material;
    invalid_material.restitution = 1.5F;
    require(world.create_body(invalid_material).error == BodyCreateError::InvalidMaterial,
            "restitution outside zero to one is rejected");

    const auto static_body = world.create_body(BodyDescription2D{
        .type = BodyType::Static,
        .position = {-5.0F, 0.0F},
        .collider = CircleCollider2D{{}, 1.0F}
    });
    require(static_cast<bool>(static_body), "static circle body creates");
    require(!world.set_velocity(static_body.handle, {1, 0}, 0),
            "static bodies do not accept velocity assignment");
    require(!world.apply_force(static_body.handle, {1, 0}),
            "static bodies do not accept force");

    const auto dynamic_body = world.create_body(BodyDescription2D{
        .type = BodyType::Dynamic,
        .position = {0, 0},
        .mass = 2.0F,
        .collider = CircleCollider2D{{}, 1.0F}
    });
    require(static_cast<bool>(dynamic_body), "dynamic circle body creates");
    require(near(world.body_state(dynamic_body.handle)->inertia, 1.0F),
            "circle inertia is mass times half radius squared");

    require(world.apply_impulse(
                dynamic_body.handle, {4, 0}, {0, 0}),
            "central impulse applies");
    auto state = world.body_state(dynamic_body.handle);
    require(state && near(state->linear_velocity.x, 2.0F),
            "central impulse changes linear velocity by impulse over mass");
    const auto movement = world.update(0.5);
    require(movement && near(
                world.body_state(dynamic_body.handle)->position.x, 1.0F),
            "semi-implicit integration advances position from velocity");

    const auto off_center = world.create_body(BodyDescription2D{
        .type = BodyType::Dynamic,
        .position = {10, 0},
        .mass = 1.0F,
        .collider = CircleCollider2D{{}, 1.0F}
    });
    require(static_cast<bool>(off_center), "off-center impulse body creates");
    require(world.apply_impulse(
                off_center.handle, {0, 2}, {11, 0}),
            "off-center impulse applies");
    state = world.body_state(off_center.handle);
    require(state && near(state->angular_velocity, 4.0F),
            "off-center impulse changes angular velocity using inertia");

    const auto kinematic = world.create_body(BodyDescription2D{
        .type = BodyType::Kinematic,
        .position = {20, 0},
        .linear_velocity = {2, 0},
        .collider = BoxCollider2D{{}, {1, 1}, 0}
    });
    require(static_cast<bool>(kinematic), "kinematic box body creates");
    const auto kinematic_step = world.update(0.25);
    require(kinematic_step &&
            near(world.body_state(kinematic.handle)->position.x, 20.5F),
            "kinematic body moves using assigned velocity");

    require(world.update(0.0), "zero delta is a valid no-op");
    require(world.update(0.25), "maximum delta is accepted");
    require(world.update(0.251).error == PhysicsStepError::DeltaTooLarge,
            "oversized delta is rejected");
    require(world.update(-0.1).error == PhysicsStepError::InvalidDelta,
            "negative delta is rejected");
    require(world.update(std::numeric_limits<double>::quiet_NaN()).error ==
                PhysicsStepError::InvalidDelta,
            "NaN delta is rejected");

    const auto stale_handle = off_center.handle;
    require(world.destroy_body(stale_handle), "body destruction succeeds");
    require(!world.destroy_body(stale_handle), "duplicate body destruction fails");
    require(!world.body_state(stale_handle), "destroyed body handle is stale");
    const auto reused = world.create_body(BodyDescription2D{
        .type = BodyType::Dynamic,
        .position = {30, 0},
        .collider = CircleCollider2D{{}, 0.5F}
    });
    require(static_cast<bool>(reused) &&
            reused.handle.index() == stale_handle.index() &&
            reused.handle.generation() != stale_handle.generation(),
            "body handle slot reuse increments generation");

    PhysicsWorld2D falling(PhysicsSettings2D{
        .gravity = {0, -9.81F},
        .velocity_iterations = 10U,
        .penetration_slop = 0.002F,
        .position_correction_percent = 0.85F
    });
    const auto ground = falling.create_body(BodyDescription2D{
        .type = BodyType::Static,
        .position = {0, -0.5F},
        .collider = BoxCollider2D{{}, {5.0F, 0.5F}, 0.0F},
        .friction = 0.8F
    });
    const auto ball = falling.create_body(BodyDescription2D{
        .type = BodyType::Dynamic,
        .position = {0, 2.0F},
        .mass = 1.0F,
        .collider = CircleCollider2D{{}, 0.5F},
        .friction = 0.8F,
        .restitution = 0.0F
    });
    require(ground && ball, "ground and falling ball are created");

    dream::core::FixedStepClock clock({1.0 / 60.0, 0.25, 8});
    std::size_t total_contacts = 0;
    for (int frame = 0; frame < 240; ++frame) {
        const auto frame_result = clock.advance(
            1.0 / 60.0, [&](double fixed_delta, std::uint64_t) {
                const auto report = falling.update(fixed_delta);
                require(static_cast<bool>(report), "fixed-step physics update succeeds");
                total_contacts += report.contact_count;
            });
        require(static_cast<bool>(frame_result), "fixed-step clock advances physics");
    }

    const auto ball_state = falling.body_state(ball.handle);
    require(ball_state && ball_state->position.y > 0.35F &&
            ball_state->position.y < 0.75F,
            "gravity and contact solver settle circle on static ground");
    require(std::abs(ball_state->linear_velocity.y) < 0.25F,
            "resting contact removes downward velocity");
    require(total_contacts > 0U, "physics reports collision contacts");

    require(falling.set_velocity(ball.handle, {4.0F, 0.0F}, 0.0F),
            "resting body accepts horizontal velocity");
    const auto friction_step = falling.update(1.0 / 60.0);
    require(friction_step && friction_step.contact_count > 0U,
            "resting body remains in contact during friction step");
    require(std::abs(falling.body_state(ball.handle)->linear_velocity.x) < 4.0F,
            "friction impulse reduces horizontal sliding velocity");

    std::cout << "Dream Engine 2D physics world tests passed.\n";
    return EXIT_SUCCESS;
}
