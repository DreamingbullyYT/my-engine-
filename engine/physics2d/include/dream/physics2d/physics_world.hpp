#pragma once

#include <dream/core/handle.hpp>
#include <dream/physics2d/broadphase.hpp>
#include <dream/physics2d/geometry.hpp>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <map>
#include <optional>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

namespace dream::physics2d {

struct BodyTag;
using BodyHandle = core::Handle<BodyTag>;

enum class BodyType : std::uint8_t {
    Static,
    Kinematic,
    Dynamic
};

struct CircleCollider2D final {
    Vec2 local_center{};
    float radius{0.5F};
};

struct BoxCollider2D final {
    Vec2 local_center{};
    Vec2 half_extents{0.5F, 0.5F};
    float local_rotation_radians{0.0F};
};

using ColliderShape2D = std::variant<CircleCollider2D, BoxCollider2D>;

struct CollisionFilter2D final {
    std::uint32_t category_bits{0x00000001U};
    std::uint32_t mask_bits{0xFFFFFFFFU};
    std::int16_t group_index{0};
};

struct BodyDescription2D final {
    BodyType type{BodyType::Static};
    Vec2 position{};
    float rotation_radians{0.0F};
    Vec2 linear_velocity{};
    float angular_velocity{0.0F};
    float mass{1.0F};
    float linear_damping{0.0F};
    float angular_damping{0.0F};
    float friction{0.5F};
    float restitution{0.0F};
    ColliderShape2D collider{CircleCollider2D{}};
    CollisionFilter2D collision_filter{};
    bool sensor{false};
};

struct PhysicsSettings2D final {
    Vec2 gravity{0.0F, -9.81F};
    std::uint32_t velocity_iterations{8U};
    std::uint32_t position_iterations{3U};
    float penetration_slop{0.005F};
    float position_correction_percent{0.8F};
    float maximum_delta_seconds{0.25F};
};

enum class BodyCreateError : std::uint8_t {
    None,
    InvalidBodyType,
    InvalidTransform,
    InvalidVelocity,
    InvalidMass,
    InvalidMaterial,
    InvalidDamping,
    InvalidCollider,
    InvalidInertia,
    InvalidWorldBounds,
    CapacityExhausted
};

enum class PhysicsStepError : std::uint8_t {
    None,
    InvalidDelta,
    DeltaTooLarge,
    InvalidSettings,
    InvalidBodyState
};

struct BodyCreateResult final {
    BodyHandle handle{};
    BodyCreateError error{BodyCreateError::None};

    [[nodiscard]] explicit operator bool() const noexcept {
        return static_cast<bool>(handle) && error == BodyCreateError::None;
    }
};

struct BodyState2D final {
    BodyHandle handle{};
    BodyType type{BodyType::Static};
    Vec2 position{};
    float rotation_radians{0.0F};
    Vec2 linear_velocity{};
    float angular_velocity{0.0F};
    float mass{0.0F};
    float inertia{0.0F};
    float friction{0.0F};
    float restitution{0.0F};
    bool sensor{false};
    CollisionFilter2D collision_filter{};
};

struct PhysicsContact2D final {
    BodyHandle first{};
    BodyHandle second{};
    ContactManifold manifold{};
    float normal_impulse{0.0F};
    float tangent_impulse{0.0F};
    bool sensor{false};
};

struct PhysicsStepReport2D final {
    PhysicsStepError error{PhysicsStepError::None};
    std::size_t body_count{0};
    std::size_t broadphase_pair_count{0};
    std::size_t contact_count{0};
    std::vector<PhysicsContact2D> contacts;

    [[nodiscard]] explicit operator bool() const noexcept {
        return error == PhysicsStepError::None;
    }
};

// A small, single-thread-owned 2D rigid-body world. One collider is supported
// per body. It uses semi-implicit Euler integration, sweep-and-prune broadphase
// and a sequential impulse normal/friction solver. Call with a fixed delta for
// stable simulation (FixedStepClock is provided by Core).
//
// Current limitations are deliberate: no joints, CCD, sleeping, persistent
// warm-start cache, multiple fixtures per body or continuous contact callbacks.
// OBB-OBB manifolds currently contain one approximate contact point.
class PhysicsWorld2D final {
public:
    explicit PhysicsWorld2D(PhysicsSettings2D settings = {}) noexcept
        : settings_(settings), settings_valid_(valid_settings(settings)) {}

    PhysicsWorld2D(const PhysicsWorld2D&) = delete;
    PhysicsWorld2D& operator=(const PhysicsWorld2D&) = delete;

    [[nodiscard]] bool valid() const noexcept { return settings_valid_; }
    [[nodiscard]] const PhysicsSettings2D& settings() const noexcept {
        return settings_;
    }

    [[nodiscard]] bool configure(PhysicsSettings2D settings) noexcept {
        if (!valid_settings(settings)) return false;
        settings_ = settings;
        settings_valid_ = true;
        return true;
    }

    [[nodiscard]] bool set_gravity(Vec2 gravity) noexcept {
        if (!finite(gravity)) return false;
        settings_.gravity = gravity;
        return true;
    }

    [[nodiscard]] std::size_t body_count() const noexcept {
        return bodies_.size();
    }

    [[nodiscard]] BodyCreateResult create_body(BodyDescription2D description) {
        const auto validation = validate_description(description);
        if (validation != BodyCreateError::None) return {{}, validation};

        const BodyHandle handle = handles_.allocate();
        if (!handle) return {{}, BodyCreateError::CapacityExhausted};

        Body body = make_body(handle, std::move(description));
        if (body.type == BodyType::Dynamic &&
            (!std::isfinite(body.inverse_mass) ||
             !std::isfinite(body.inertia) ||
             !std::isfinite(body.inverse_inertia) ||
             body.inverse_mass <= 0.0F ||
             body.inertia <= 1.0e-12F ||
             body.inverse_inertia <= 0.0F)) {
            (void)handles_.release(handle);
            return {{}, BodyCreateError::InvalidInertia};
        }

        try {
            const auto [inserted_at, inserted] =
                bodies_.emplace(handle.index(), std::move(body));
            if (!inserted) {
                (void)handles_.release(handle);
                return {{}, BodyCreateError::CapacityExhausted};
            }
            if (!update_proxy(inserted_at->second)) {
                bodies_.erase(inserted_at);
                (void)handles_.release(handle);
                return {{}, BodyCreateError::InvalidWorldBounds};
            }
        } catch (...) {
            (void)handles_.release(handle);
            throw;
        }
        return {handle, BodyCreateError::None};
    }

    [[nodiscard]] bool destroy_body(BodyHandle handle) {
        if (!is_alive(handle)) return false;
        if (!handles_.release(handle)) return false;
        bodies_.erase(handle.index());
        (void)broadphase_.remove(proxy_id(handle));
        return true;
    }

    [[nodiscard]] bool is_alive(BodyHandle handle) const noexcept {
        if (!handle) return false;
        const auto found = bodies_.find(handle.index());
        return found != bodies_.end() && found->second.handle == handle;
    }

    [[nodiscard]] std::optional<BodyState2D> body_state(BodyHandle handle) const {
        const auto* body = find_body(handle);
        if (body == nullptr) return std::nullopt;
        return BodyState2D{
            body->handle, body->type, body->position, body->rotation,
            body->velocity, body->angular_velocity, body->mass,
            body->inertia, body->friction, body->restitution,
            body->sensor, body->collision_filter
        };
    }

    [[nodiscard]] bool set_transform(
        BodyHandle handle, Vec2 position, float rotation_radians) {
        auto* body = find_body(handle);
        if (body == nullptr || !finite(position) ||
            !std::isfinite(rotation_radians)) return false;
        const Vec2 previous_position = body->position;
        const float previous_rotation = body->rotation;
        body->position = position;
        body->rotation = rotation_radians;
        if (update_proxy(*body)) return true;

        body->position = previous_position;
        body->rotation = previous_rotation;
        return false;
    }

    [[nodiscard]] bool set_velocity(
        BodyHandle handle, Vec2 linear_velocity, float angular_velocity) {
        auto* body = find_body(handle);
        if (body == nullptr || body->type == BodyType::Static ||
            !finite(linear_velocity) || !std::isfinite(angular_velocity)) {
            return false;
        }
        body->velocity = linear_velocity;
        body->angular_velocity = angular_velocity;
        return true;
    }

    [[nodiscard]] bool apply_force(BodyHandle handle, Vec2 force) noexcept {
        auto* body = find_body(handle);
        if (body == nullptr || body->type != BodyType::Dynamic ||
            !finite(force)) return false;
        const Vec2 next_force = body->force + force;
        if (!finite(next_force)) return false;
        body->force = next_force;
        return true;
    }

    [[nodiscard]] bool apply_torque(BodyHandle handle, float torque) noexcept {
        auto* body = find_body(handle);
        if (body == nullptr || body->type != BodyType::Dynamic ||
            !std::isfinite(torque) ||
            !std::isfinite(body->torque + torque)) return false;
        body->torque += torque;
        return true;
    }

    [[nodiscard]] bool apply_impulse(
        BodyHandle handle, Vec2 impulse, Vec2 world_point) noexcept {
        auto* body = find_body(handle);
        if (body == nullptr || body->type != BodyType::Dynamic ||
            !finite(impulse) || !finite(world_point)) return false;

        const Vec2 next_velocity =
            body->velocity + impulse * body->inverse_mass;
        const float angular_delta =
            cross(world_point - body->position, impulse) * body->inverse_inertia;
        if (!finite(next_velocity) ||
            !std::isfinite(body->angular_velocity + angular_delta)) return false;

        body->velocity = next_velocity;
        body->angular_velocity += angular_delta;
        return true;
    }

    [[nodiscard]] PhysicsStepReport2D update(double delta_seconds) {
        PhysicsStepReport2D report;
        report.body_count = bodies_.size();

        if (!settings_valid_) {
            report.error = PhysicsStepError::InvalidSettings;
            return report;
        }
        if (!std::isfinite(delta_seconds) || delta_seconds < 0.0) {
            report.error = PhysicsStepError::InvalidDelta;
            return report;
        }
        if (delta_seconds > settings_.maximum_delta_seconds) {
            report.error = PhysicsStepError::DeltaTooLarge;
            return report;
        }
        if (delta_seconds == 0.0) return report;

        const float delta = static_cast<float>(delta_seconds);
        for (auto& [index, body] : bodies_) {
            (void)index;
            if (body.type == BodyType::Dynamic) {
                body.velocity = body.velocity +
                    (settings_.gravity + body.force * body.inverse_mass) * delta;
                body.angular_velocity += body.torque * body.inverse_inertia * delta;
            }

            if (body.type != BodyType::Static) {
                const float linear_factor =
                    1.0F / (1.0F + body.linear_damping * delta);
                const float angular_factor =
                    1.0F / (1.0F + body.angular_damping * delta);
                body.velocity = body.velocity * linear_factor;
                body.angular_velocity *= angular_factor;
                body.position = body.position + body.velocity * delta;
                body.rotation += body.angular_velocity * delta;
            }

            body.force = {};
            body.torque = 0.0F;
            if (!finite(body.position) || !finite(body.velocity) ||
                !std::isfinite(body.rotation) ||
                !std::isfinite(body.angular_velocity)) {
                report.error = PhysicsStepError::InvalidBodyState;
                return report;
            }
        }

        for (auto& [index, body] : bodies_) {
            (void)index;
            if (!update_proxy(body)) {
                report.error = PhysicsStepError::InvalidBodyState;
                return report;
            }
        }

        const auto pairs = broadphase_.compute_pairs();
        report.broadphase_pair_count = pairs.pairs.size();

        std::vector<SolverContact> contacts;
        contacts.reserve(pairs.pairs.size());
        for (const auto& pair : pairs.pairs) {
            auto* first = find_proxy(pair.first);
            auto* second = find_proxy(pair.second);
            if (first == nullptr || second == nullptr) continue;
            if (!should_collide(first->collision_filter, second->collision_filter))
                continue;
            const bool sensor_pair = first->sensor || second->sensor;
            if (!sensor_pair && first->inverse_mass == 0.0F &&
                second->inverse_mass == 0.0F) continue;

            const auto manifold = collide_bodies(*first, *second);
            if (!manifold) continue;
            contacts.push_back({
                first->handle, second->handle, *manifold, 0.0F, 0.0F,
                sensor_pair
            });
        }

        for (std::uint32_t iteration = 0;
             iteration < settings_.velocity_iterations; ++iteration) {
            for (auto& contact : contacts) {
                auto* first = find_body(contact.first);
                auto* second = find_body(contact.second);
                if (first == nullptr || second == nullptr || contact.sensor) continue;
                solve_velocity(*first, *second, contact);
            }
        }

        for (std::uint32_t iteration = 0;
             iteration < settings_.position_iterations; ++iteration) {
            for (auto& contact : contacts) {
                if (contact.sensor) continue;
                auto* first = find_body(contact.first);
                auto* second = find_body(contact.second);
                if (first == nullptr || second == nullptr) continue;
                const auto manifold = collide_bodies(*first, *second);
                if (!manifold) continue;
                contact.manifold = *manifold;
                solve_position(*first, *second, *manifold);
            }
        }

        for (auto& [index, body] : bodies_) {
            (void)index;
            if (!update_proxy(body)) {
                report.error = PhysicsStepError::InvalidBodyState;
                return report;
            }
        }

        report.contacts.reserve(contacts.size());
        for (const auto& contact : contacts) {
            report.contacts.push_back({
                contact.first, contact.second, contact.manifold,
                contact.normal_impulse, contact.tangent_impulse,
                contact.sensor
            });
        }
        report.contact_count = report.contacts.size();
        return report;
    }

private:
    struct Body final {
        BodyHandle handle{};
        BodyType type{BodyType::Static};
        Vec2 position{};
        float rotation{0.0F};
        Vec2 velocity{};
        float angular_velocity{0.0F};
        Vec2 force{};
        float torque{0.0F};
        float mass{1.0F};
        float inverse_mass{0.0F};
        float inertia{1.0F};
        float inverse_inertia{0.0F};
        float linear_damping{0.0F};
        float angular_damping{0.0F};
        float friction{0.5F};
        float restitution{0.0F};
        ColliderShape2D collider{CircleCollider2D{}};
        CollisionFilter2D collision_filter{};
        bool sensor{false};
    };

    using WorldShape = std::variant<Circle, OrientedBox>;

    struct SolverContact final {
        BodyHandle first{};
        BodyHandle second{};
        ContactManifold manifold{};
        float normal_impulse{0.0F};
        float tangent_impulse{0.0F};
        bool sensor{false};
    };

    [[nodiscard]] static bool should_collide(
        CollisionFilter2D first, CollisionFilter2D second) noexcept {
        if (first.group_index != 0 &&
            first.group_index == second.group_index) {
            return first.group_index > 0;
        }
        return (first.mask_bits & second.category_bits) != 0U &&
               (second.mask_bits & first.category_bits) != 0U;
    }

    [[nodiscard]] static bool finite(Vec2 value) noexcept {
        return std::isfinite(value.x) && std::isfinite(value.y);
    }

    [[nodiscard]] static bool valid_settings(
        const PhysicsSettings2D& settings) noexcept {
        return finite(settings.gravity) &&
               settings.velocity_iterations > 0U &&
               settings.velocity_iterations <= 64U &&
               settings.position_iterations > 0U &&
               settings.position_iterations <= 32U &&
               std::isfinite(settings.penetration_slop) &&
               settings.penetration_slop >= 0.0F &&
               std::isfinite(settings.position_correction_percent) &&
               settings.position_correction_percent >= 0.0F &&
               settings.position_correction_percent <= 1.0F &&
               std::isfinite(settings.maximum_delta_seconds) &&
               settings.maximum_delta_seconds > 0.0F;
    }

    [[nodiscard]] static BodyCreateError validate_description(
        const BodyDescription2D& description) noexcept {
        switch (description.type) {
        case BodyType::Static:
        case BodyType::Kinematic:
        case BodyType::Dynamic:
            break;
        default:
            return BodyCreateError::InvalidBodyType;
        }

        if (!finite(description.position) ||
            !std::isfinite(description.rotation_radians)) {
            return BodyCreateError::InvalidTransform;
        }
        if (!finite(description.linear_velocity) ||
            !std::isfinite(description.angular_velocity)) {
            return BodyCreateError::InvalidVelocity;
        }
        if (!std::isfinite(description.mass) || description.mass <= 0.0F) {
            return BodyCreateError::InvalidMass;
        }
        if (!std::isfinite(description.friction) || description.friction < 0.0F ||
            !std::isfinite(description.restitution) ||
            description.restitution < 0.0F || description.restitution > 1.0F) {
            return BodyCreateError::InvalidMaterial;
        }
        if (!std::isfinite(description.linear_damping) ||
            description.linear_damping < 0.0F ||
            !std::isfinite(description.angular_damping) ||
            description.angular_damping < 0.0F) {
            return BodyCreateError::InvalidDamping;
        }

        const bool collider_valid = std::visit([](const auto& collider) {
            using Collider = std::decay_t<decltype(collider)>;
            if (!finite(collider.local_center)) return false;
            if constexpr (std::is_same_v<Collider, CircleCollider2D>) {
                return std::isfinite(collider.radius) && collider.radius > 0.0F;
            } else {
                return finite(collider.half_extents) &&
                       collider.half_extents.x > 0.0F &&
                       collider.half_extents.y > 0.0F &&
                       std::isfinite(collider.local_rotation_radians);
            }
        }, description.collider);

        return collider_valid
            ? BodyCreateError::None : BodyCreateError::InvalidCollider;
    }

    [[nodiscard]] static Body make_body(
        BodyHandle handle, BodyDescription2D description) noexcept {
        Body body;
        body.handle = handle;
        body.type = description.type;
        body.position = description.position;
        body.rotation = description.rotation_radians;
        body.velocity = description.linear_velocity;
        body.angular_velocity = description.angular_velocity;
        body.mass = description.mass;
        body.linear_damping = description.linear_damping;
        body.angular_damping = description.angular_damping;
        body.friction = description.friction;
        body.restitution = description.restitution;
        body.collider = std::move(description.collider);
        body.collision_filter = description.collision_filter;
        body.sensor = description.sensor;

        const float inertia = std::visit([&](const auto& collider) {
            using Collider = std::decay_t<decltype(collider)>;
            const float offset_squared = length_squared(collider.local_center);
            if constexpr (std::is_same_v<Collider, CircleCollider2D>) {
                return body.mass *
                    (0.5F * collider.radius * collider.radius + offset_squared);
            } else {
                const float width = collider.half_extents.x * 2.0F;
                const float height = collider.half_extents.y * 2.0F;
                return body.mass *
                    ((width * width + height * height) / 12.0F + offset_squared);
            }
        }, body.collider);

        body.inertia = inertia;
        if (body.type == BodyType::Dynamic) {
            body.inverse_mass = 1.0F / body.mass;
            body.inverse_inertia = inertia > 1.0e-12F ? 1.0F / inertia : 0.0F;
        }
        return body;
    }

    [[nodiscard]] static ColliderProxyId proxy_id(BodyHandle handle) noexcept {
        return (static_cast<std::uint64_t>(handle.index()) << 32U) |
               static_cast<std::uint64_t>(handle.generation());
    }

    [[nodiscard]] WorldShape world_shape(const Body& body) const noexcept {
        return std::visit([&](const auto& collider) -> WorldShape {
            using Collider = std::decay_t<decltype(collider)>;
            const Vec2 center = body.position +
                rotate(collider.local_center, body.rotation);
            if constexpr (std::is_same_v<Collider, CircleCollider2D>) {
                return Circle{center, collider.radius};
            } else {
                return OrientedBox{
                    center, collider.half_extents,
                    body.rotation + collider.local_rotation_radians
                };
            }
        }, body.collider);
    }

    [[nodiscard]] Aabb world_bounds(const Body& body) const noexcept {
        return std::visit([](const auto& shape) {
            return shape.bounds();
        }, world_shape(body));
    }

    [[nodiscard]] bool update_proxy(const Body& body) {
        return broadphase_.upsert(proxy_id(body.handle), world_bounds(body)) ==
               BroadphaseError::None;
    }

    [[nodiscard]] Body* find_body(BodyHandle handle) noexcept {
        if (!handle) return nullptr;
        const auto found = bodies_.find(handle.index());
        if (found == bodies_.end() || found->second.handle != handle)
            return nullptr;
        return &found->second;
    }

    [[nodiscard]] const Body* find_body(BodyHandle handle) const noexcept {
        if (!handle) return nullptr;
        const auto found = bodies_.find(handle.index());
        if (found == bodies_.end() || found->second.handle != handle)
            return nullptr;
        return &found->second;
    }

    [[nodiscard]] Body* find_proxy(ColliderProxyId id) noexcept {
        const auto index = static_cast<std::uint32_t>(id >> 32U);
        const auto generation = static_cast<std::uint32_t>(id & 0xFFFFFFFFULL);
        const auto found = bodies_.find(index);
        if (found == bodies_.end() ||
            found->second.handle.generation() != generation) return nullptr;
        return &found->second;
    }

    [[nodiscard]] static std::optional<ContactManifold> collide_bodies(
        const Body& first, const Body& second) noexcept {
        const WorldShape first_shape = [&] {
            return std::visit([&](const auto& collider) -> WorldShape {
                using Collider = std::decay_t<decltype(collider)>;
                const Vec2 center = first.position +
                    rotate(collider.local_center, first.rotation);
                if constexpr (std::is_same_v<Collider, CircleCollider2D>)
                    return Circle{center, collider.radius};
                else
                    return OrientedBox{
                        center, collider.half_extents,
                        first.rotation + collider.local_rotation_radians
                    };
            }, first.collider);
        }();

        const WorldShape second_shape = [&] {
            return std::visit([&](const auto& collider) -> WorldShape {
                using Collider = std::decay_t<decltype(collider)>;
                const Vec2 center = second.position +
                    rotate(collider.local_center, second.rotation);
                if constexpr (std::is_same_v<Collider, CircleCollider2D>)
                    return Circle{center, collider.radius};
                else
                    return OrientedBox{
                        center, collider.half_extents,
                        second.rotation + collider.local_rotation_radians
                    };
            }, second.collider);
        }();

        return std::visit([](const auto& left, const auto& right) {
            return collide(left, right);
        }, first_shape, second_shape);
    }

    static Vec2 point_velocity(const Body& body, Vec2 radius) noexcept {
        return body.velocity + Vec2{
            -body.angular_velocity * radius.y,
             body.angular_velocity * radius.x
        };
    }

    static void apply_pair_impulse(
        Body& first, Body& second, Vec2 impulse,
        Vec2 first_radius, Vec2 second_radius) noexcept {
        first.velocity = first.velocity -
            impulse * first.inverse_mass;
        first.angular_velocity -=
            cross(first_radius, impulse) * first.inverse_inertia;
        second.velocity = second.velocity +
            impulse * second.inverse_mass;
        second.angular_velocity +=
            cross(second_radius, impulse) * second.inverse_inertia;
    }

    static void solve_velocity(
        Body& first, Body& second, SolverContact& contact) noexcept {
        const Vec2 normal = contact.manifold.normal;
        const Vec2 first_radius = contact.manifold.point - first.position;
        const Vec2 second_radius = contact.manifold.point - second.position;
        const float first_cross = cross(first_radius, normal);
        const float second_cross = cross(second_radius, normal);
        const float denominator =
            first.inverse_mass + second.inverse_mass +
            first_cross * first_cross * first.inverse_inertia +
            second_cross * second_cross * second.inverse_inertia;
        if (denominator <= 1.0e-12F) return;

        Vec2 relative_velocity =
            point_velocity(second, second_radius) -
            point_velocity(first, first_radius);
        const float normal_speed = dot(relative_velocity, normal);
        if (normal_speed < 0.0F) {
            const float restitution =
                std::abs(normal_speed) > 0.5F
                    ? std::max(first.restitution, second.restitution) : 0.0F;
            const float impulse_delta =
                -(1.0F + restitution) * normal_speed / denominator;
            const float old_normal_impulse = contact.normal_impulse;
            contact.normal_impulse = std::max(
                old_normal_impulse + impulse_delta, 0.0F);
            const float applied_normal =
                contact.normal_impulse - old_normal_impulse;
            apply_pair_impulse(
                first, second, normal * applied_normal,
                first_radius, second_radius);
        }

        if (contact.normal_impulse <= 0.0F) return;
        relative_velocity =
            point_velocity(second, second_radius) -
            point_velocity(first, first_radius);
        Vec2 tangent = relative_velocity -
            normal * dot(relative_velocity, normal);
        const float tangent_length = length(tangent);
        if (tangent_length <= 1.0e-8F) return;
        tangent = tangent / tangent_length;

        const float first_tangent_cross = cross(first_radius, tangent);
        const float second_tangent_cross = cross(second_radius, tangent);
        const float tangent_denominator =
            first.inverse_mass + second.inverse_mass +
            first_tangent_cross * first_tangent_cross * first.inverse_inertia +
            second_tangent_cross * second_tangent_cross * second.inverse_inertia;
        if (tangent_denominator <= 1.0e-12F) return;

        const float friction_delta =
            -dot(relative_velocity, tangent) / tangent_denominator;
        const float friction =
            std::sqrt(first.friction) * std::sqrt(second.friction);
        const float maximum_friction = friction * contact.normal_impulse;
        const float old_tangent_impulse = contact.tangent_impulse;
        contact.tangent_impulse = std::clamp(
            old_tangent_impulse + friction_delta,
            -maximum_friction, maximum_friction);
        const float applied_friction =
            contact.tangent_impulse - old_tangent_impulse;
        apply_pair_impulse(
            first, second, tangent * applied_friction,
            first_radius, second_radius);
    }

    void solve_position(
        Body& first, Body& second, const ContactManifold& manifold) const noexcept {
        const float inverse_mass_sum =
            first.inverse_mass + second.inverse_mass;
        if (inverse_mass_sum <= 1.0e-12F) return;

        const float correction_magnitude =
            std::max(manifold.penetration - settings_.penetration_slop, 0.0F) *
            settings_.position_correction_percent / inverse_mass_sum;
        const Vec2 correction = manifold.normal * correction_magnitude;
        first.position = first.position -
            correction * first.inverse_mass;
        second.position = second.position +
            correction * second.inverse_mass;
    }

    PhysicsSettings2D settings_{};
    bool settings_valid_{true};
    core::HandlePool<BodyTag> handles_;
    std::map<std::uint32_t, Body> bodies_;
    SweepAndPrune broadphase_;
};

} // namespace dream::physics2d
