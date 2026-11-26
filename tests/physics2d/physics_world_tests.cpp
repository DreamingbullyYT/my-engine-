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
    const auto movement_first = world.update(0.25);
    const auto movement_second = world.update(0.25);
    require(static_cast<bool>(movement_first) &&
            static_cast<bool>(movement_second) &&
            near(world.body_state(dynamic_body.handle)->position.x, 1.0F),
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

    require(static_cast<bool>(world.update(0.0)), "zero delta is a valid no-op");
    require(static_cast<bool>(world.update(0.25)), "maximum delta is accepted");
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
        .friction = 0.8F,
        .collider = BoxCollider2D{{}, {5.0F, 0.5F}, 0.0F}
    });
    const auto ball = falling.create_body(BodyDescription2D{
        .type = BodyType::Dynamic,
        .position = {0, 2.0F},
        .mass = 1.0F,
        .friction = 0.8F,
        .restitution = 0.0F,
        .collider = CircleCollider2D{{}, 0.5F}
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

    PhysicsWorld2D filtering(PhysicsSettings2D{
        .gravity = {0, 0},
        .velocity_iterations = 8U
    });
    CollisionFilter2D category_a{0x1U, 0x2U, 0};
    CollisionFilter2D category_b{0x2U, 0x1U, 0};
    const auto filtered_a = filtering.create_body(BodyDescription2D{
        .type = BodyType::Dynamic,
        .position = {0, 0},
        .mass = 1.0F,
        .collider = CircleCollider2D{{}, 1.0F},
        .collision_filter = category_a
    });
    const auto filtered_b = filtering.create_body(BodyDescription2D{
        .type = BodyType::Static,
        .position = {1, 0},
        .collider = CircleCollider2D{{}, 1.0F},
        .collision_filter = category_b
    });
    require(filtered_a && filtered_b, "filtered bodies are created");
    auto filtered_report = filtering.update(1.0 / 60.0);
    require(filtered_report && filtered_report.contact_count == 1U,
            "mutually permitted category and mask bits collide");

    PhysicsWorld2D mask_filtering(PhysicsSettings2D{.gravity = {0, 0}});
    const auto mask_a = mask_filtering.create_body(BodyDescription2D{
        .type = BodyType::Dynamic,
        .position = {0, 0},
        .mass = 1.0F,
        .collider = CircleCollider2D{{}, 1.0F},
        .collision_filter = CollisionFilter2D{0x1U, 0x2U, 0}
    });
    const auto mask_b = mask_filtering.create_body(BodyDescription2D{
        .type = BodyType::Static,
        .position = {1, 0},
        .collider = CircleCollider2D{{}, 1.0F},
        .collision_filter = CollisionFilter2D{0x2U, 0x4U, 0}
    });
    require(mask_a && mask_b, "mask-filter bodies are created");
    const auto denied_report = mask_filtering.update(1.0 / 60.0);
    require(denied_report && denied_report.contact_count == 0U,
            "collision is rejected unless both masks permit it");

    PhysicsWorld2D grouped(PhysicsSettings2D{.gravity = {0, 0}});
    const auto group_a = grouped.create_body(BodyDescription2D{
        .type = BodyType::Dynamic,
        .position = {0, 0},
        .mass = 1.0F,
        .collider = CircleCollider2D{{}, 1.0F},
        .collision_filter = CollisionFilter2D{0x1U, 0x0U, 7}
    });
    const auto group_b = grouped.create_body(BodyDescription2D{
        .type = BodyType::Static,
        .position = {1, 0},
        .collider = CircleCollider2D{{}, 1.0F},
        .collision_filter = CollisionFilter2D{0x2U, 0x0U, 7}
    });
    require(group_a && group_b, "positive-group bodies are created");
    require(grouped.update(1.0 / 60.0).contact_count == 1U,
            "same positive group overrides masks to allow collision");

    PhysicsWorld2D ignored_group(PhysicsSettings2D{.gravity = {0, 0}});
    const auto ignored_a = ignored_group.create_body(BodyDescription2D{
        .type = BodyType::Dynamic,
        .position = {0, 0},
        .mass = 1.0F,
        .collider = CircleCollider2D{{}, 1.0F},
        .collision_filter = CollisionFilter2D{0x1U, 0xFFFFFFFFU, -3}
    });
    const auto ignored_b = ignored_group.create_body(BodyDescription2D{
        .type = BodyType::Static,
        .position = {1, 0},
        .collider = CircleCollider2D{{}, 1.0F},
        .collision_filter = CollisionFilter2D{0x1U, 0xFFFFFFFFU, -3}
    });
    require(ignored_a && ignored_b, "negative-group bodies are created");
    require(ignored_group.update(1.0 / 60.0).contact_count == 0U,
            "same negative group suppresses collision");

    PhysicsWorld2D sensors(PhysicsSettings2D{.gravity = {0, 0}});
    const auto sensor = sensors.create_body(BodyDescription2D{
        .type = BodyType::Static,
        .position = {0, 0},
        .collider = CircleCollider2D{{}, 1.0F},
        .sensor = true
    });
    const auto sensor_body = sensors.create_body(BodyDescription2D{
        .type = BodyType::Dynamic,
        .position = {1, 0},
        .linear_velocity = {-2, 0},
        .mass = 1.0F,
        .collider = CircleCollider2D{{}, 1.0F}
    });
    require(sensor && sensor_body, "sensor pair is created");
    const auto sensor_before = sensors.body_state(sensor_body.handle);
    const auto sensor_report = sensors.update(1.0 / 60.0);
    const auto sensor_after = sensors.body_state(sensor_body.handle);
    require(sensor_report && sensor_report.contact_count == 1U &&
            sensor_report.contacts[0].sensor,
            "sensor overlap is reported as a trigger contact");
    require(sensor_after && sensor_before &&
            sensor_after->linear_velocity.x == sensor_before->linear_velocity.x,
            "sensor contact applies no collision impulse");
    require(sensor_report.events.size() == 1U &&
            sensor_report.events[0].type == ContactEventType2D::Begin,
            "new sensor overlap emits Begin event");

    const auto sensor_stay = sensors.update(1.0 / 60.0);
    require(sensor_stay.events.size() == 1U &&
            sensor_stay.events[0].type == ContactEventType2D::Stay,
            "persistent sensor overlap emits Stay event");
    require(sensors.set_transform(sensor_body.handle, {10, 0}, 0),
            "moving trigger body out of overlap succeeds");
    const auto sensor_end = sensors.update(1.0 / 60.0);
    require(sensor_end.contact_count == 0U &&
            sensor_end.events.size() == 1U &&
            sensor_end.events[0].type == ContactEventType2D::End,
            "separated sensor pair emits End event");

    PhysicsWorld2D static_sensors(PhysicsSettings2D{.gravity = {0, 0}});
    const auto static_sensor_a = static_sensors.create_body(BodyDescription2D{
        .type = BodyType::Static,
        .position = {0, 0},
        .collider = CircleCollider2D{{}, 1.0F},
        .sensor = true
    });
    const auto static_sensor_b = static_sensors.create_body(BodyDescription2D{
        .type = BodyType::Static,
        .position = {1, 0},
        .collider = CircleCollider2D{{}, 1.0F}
    });
    require(static_sensor_a && static_sensor_b,
            "static sensor pair can be created");
    const auto static_sensor_report = static_sensors.update(1.0 / 60.0);
    require(static_sensor_report.contact_count == 1U &&
            static_sensor_report.contacts[0].sensor &&
            static_sensor_report.events.size() == 1U &&
            static_sensor_report.events[0].type == ContactEventType2D::Begin,
            "static sensor overlaps are reported as contact events");

    PhysicsWorld2D queries(PhysicsSettings2D{.gravity = {0, 0}});
    const auto near_circle = queries.create_body(BodyDescription2D{
        .type = BodyType::Static,
        .position = {5.0F, 0.0F},
        .collider = CircleCollider2D{{}, 1.0F},
        .collision_filter = CollisionFilter2D{0x1U, 0xFFFFFFFFU, 0}
    });
    const auto far_box = queries.create_body(BodyDescription2D{
        .type = BodyType::Static,
        .position = {8.0F, 0.0F},
        .collider = BoxCollider2D{{}, {1.0F, 2.0F}, 0.3F},
        .collision_filter = CollisionFilter2D{0x2U, 0xFFFFFFFFU, 0},
        .sensor = true
    });
    require(near_circle && far_box, "ray query colliders create");
    const auto ray_hits = queries.raycast_all(
        Ray2D{{0, 0}, {2, 0}, 20.0F});
    require(ray_hits.size() == 2U &&
            ray_hits[0].body == near_circle.handle &&
            ray_hits[1].body == far_box.handle,
            "raycast-all returns circle and rotated box ordered by distance");
    require(near(ray_hits[0].distance, 4.0F) &&
            near(ray_hits[0].point.x, 4.0F) &&
            near(ray_hits[0].normal.x, -1.0F),
            "circle raycast returns nearest world-space hit");
    require(ray_hits[1].sensor && near(ray_hits[1].distance, 7.0F, 0.01F),
            "rotated OBB raycast reports sensor and world distance");
    const auto nearest_hit = queries.raycast(Ray2D{{0, 0}, {1, 0}, 20.0F});
    require(nearest_hit && nearest_hit->body == near_circle.handle,
            "single raycast returns the nearest collider");

    const auto filtered_hits = queries.raycast_all(
        Ray2D{{0, 0}, {1, 0}, 20.0F}, RaycastFilter2D{0x1U, 0x1U});
    require(filtered_hits.size() == 1U &&
            filtered_hits[0].body == near_circle.handle,
            "raycast respects category and mask query filtering");

    const auto inside_hit = queries.raycast(
        Ray2D{{5, 0}, {1, 0}, 20.0F});
    require(inside_hit && inside_hit->body == near_circle.handle &&
            inside_hit->distance == 0.0F &&
            near(inside_hit->normal.x, -1.0F),
            "ray beginning inside a collider returns a zero-distance hit");
    require(queries.raycast(Ray2D{{0, 0}, {0, 0}, 20.0F}) == std::nullopt,
            "zero-direction ray is rejected");
    require(queries.raycast(Ray2D{{0, 0}, {1, 0}, -1.0F}) == std::nullopt,
            "negative ray distance is rejected");

    const auto point_hits = queries.overlap_point({5.0F, 0.0F});
    require(point_hits.size() == 1U && point_hits[0] == near_circle.handle,
            "point overlap query finds containing circle");
    const auto rotated_box_point = rotate(
        Vec2{0.5F, 0.0F}, 0.3F) + Vec2{8.0F, 0.0F};
    const auto box_point_hits = queries.overlap_point(rotated_box_point);
    require(box_point_hits.size() == 1U && box_point_hits[0] == far_box.handle,
            "point overlap query handles rotated boxes");
    require(queries.overlap_point({100, 100}).empty(),
            "point outside every collider returns no results");

    PhysicsWorld2D invalid_bounds(PhysicsSettings2D{.gravity = {0, 0}});
    const float huge = std::numeric_limits<float>::max() * 0.75F;
    const auto edge_body = invalid_bounds.create_body(BodyDescription2D{
        .type = BodyType::Static,
        .position = {0, 0},
        .collider = CircleCollider2D{{}, huge}
    });
    require(static_cast<bool>(edge_body),
            "large but finite static collider has valid initial bounds");
    const auto edge_state_before = invalid_bounds.body_state(edge_body.handle);
    require(!invalid_bounds.set_transform(edge_body.handle, {huge, 0}, 0),
            "transform producing infinite collider bounds is rejected");
    const auto edge_state_after = invalid_bounds.body_state(edge_body.handle);
    require(edge_state_before && edge_state_after &&
            edge_state_after->position.x == edge_state_before->position.x,
            "failed transform update restores the previous body transform");

    const auto overflowing_body = invalid_bounds.create_body(BodyDescription2D{
        .type = BodyType::Static,
        .position = {huge, 0},
        .collider = CircleCollider2D{{}, huge}
    });
    require(overflowing_body.error == BodyCreateError::InvalidWorldBounds,
            "body with overflowing initial world bounds is rejected");

    const auto tiny_dynamic = invalid_bounds.create_body(BodyDescription2D{
        .type = BodyType::Dynamic,
        .position = {0, 10},
        .mass = 1.0F,
        .collider = CircleCollider2D{{}, 1.0e-12F}
    });
    require(tiny_dynamic.error == BodyCreateError::InvalidInertia,
            "dynamic body with unusably small inertia is rejected");

    PhysicsWorld2D transactional(PhysicsSettings2D{.gravity = {0, 0}});
    const float near_limit = std::numeric_limits<float>::max() * 0.9F;
    const auto overflow_candidate = transactional.create_body(BodyDescription2D{
        .type = BodyType::Dynamic,
        .position = {near_limit, 0},
        .mass = 1.0F,
        .collider = CircleCollider2D{{}, 1.0F}
    });
    require(static_cast<bool>(overflow_candidate),
            "large but finite dynamic body can be created");
    require(transactional.set_velocity(
                overflow_candidate.handle, {near_limit, 0}, 0),
            "large finite velocity can be assigned");
    const auto before_overflow = transactional.body_state(
        overflow_candidate.handle);
    const auto overflow_step = transactional.update(0.25);
    const auto after_overflow = transactional.body_state(
        overflow_candidate.handle);
    require(overflow_step.error == PhysicsStepError::InvalidBodyState,
            "integration rejects a world-space position overflow");
    require(before_overflow && after_overflow &&
            after_overflow->position.x == before_overflow->position.x &&
            after_overflow->linear_velocity.x == before_overflow->linear_velocity.x,
            "failed integration leaves every body state unchanged");

    std::cout << "Dream Engine 2D physics world tests passed.\n";
    return EXIT_SUCCESS;
}
