#pragma once

#include <dream/world/world.hpp>

#include <cmath>

namespace dream::world {

struct Vec2 final {
    float x{0.0F};
    float y{0.0F};
};

struct Vec3 final {
    float x{0.0F};
    float y{0.0F};
    float z{0.0F};
};

struct Quaternion final {
    float x{0.0F};
    float y{0.0F};
    float z{0.0F};
    float w{1.0F};
};

// Data-only transform components. 2D rotation is radians; 3D rotation is a
// quaternion. Composition assumes normalized 3D quaternions.
struct Transform2D final {
    Vec2 position{};
    float rotation_radians{0.0F};
    Vec2 scale{1.0F, 1.0F};
};

struct Transform3D final {
    Vec3 position{};
    Quaternion rotation{};
    Vec3 scale{1.0F, 1.0F, 1.0F};
};

struct Parent final {
    Entity entity{};

    [[nodiscard]] constexpr bool has_parent() const noexcept {
        return static_cast<bool>(entity);
    }
};

[[nodiscard]] inline Vec2 rotate(Vec2 value, float radians) noexcept {
    const float cosine = std::cos(radians);
    const float sine = std::sin(radians);
    return {
        value.x * cosine - value.y * sine,
        value.x * sine + value.y * cosine
    };
}

[[nodiscard]] inline Transform2D compose(
    const Transform2D& parent, const Transform2D& local) noexcept {
    const Vec2 scaled_local{
        local.position.x * parent.scale.x,
        local.position.y * parent.scale.y
    };
    const Vec2 rotated_local = rotate(scaled_local, parent.rotation_radians);
    return {
        {parent.position.x + rotated_local.x,
         parent.position.y + rotated_local.y},
        parent.rotation_radians + local.rotation_radians,
        {parent.scale.x * local.scale.x, parent.scale.y * local.scale.y}
    };
}

[[nodiscard]] inline Quaternion multiply(
    Quaternion left, Quaternion right) noexcept {
    return {
        left.w * right.x + left.x * right.w + left.y * right.z - left.z * right.y,
        left.w * right.y - left.x * right.z + left.y * right.w + left.z * right.x,
        left.w * right.z + left.x * right.y - left.y * right.x + left.z * right.w,
        left.w * right.w - left.x * right.x - left.y * right.y - left.z * right.z
    };
}

// Rotates a vector using a normalized quaternion.
[[nodiscard]] inline Vec3 rotate(Vec3 value, Quaternion rotation) noexcept {
    const Vec3 axis{rotation.x, rotation.y, rotation.z};
    const Vec3 cross_one{
        axis.y * value.z - axis.z * value.y,
        axis.z * value.x - axis.x * value.z,
        axis.x * value.y - axis.y * value.x
    };
    const Vec3 cross_two{
        axis.y * cross_one.z - axis.z * cross_one.y,
        axis.z * cross_one.x - axis.x * cross_one.z,
        axis.x * cross_one.y - axis.y * cross_one.x
    };
    return {
        value.x + 2.0F * (rotation.w * cross_one.x + cross_two.x),
        value.y + 2.0F * (rotation.w * cross_one.y + cross_two.y),
        value.z + 2.0F * (rotation.w * cross_one.z + cross_two.z)
    };
}

[[nodiscard]] inline Transform3D compose(
    const Transform3D& parent, const Transform3D& local) noexcept {
    const Vec3 scaled_local{
        local.position.x * parent.scale.x,
        local.position.y * parent.scale.y,
        local.position.z * parent.scale.z
    };
    const Vec3 rotated_local = rotate(scaled_local, parent.rotation);
    return {
        {parent.position.x + rotated_local.x,
         parent.position.y + rotated_local.y,
         parent.position.z + rotated_local.z},
        multiply(parent.rotation, local.rotation),
        {parent.scale.x * local.scale.x,
         parent.scale.y * local.scale.y,
         parent.scale.z * local.scale.z}
    };
}

} // namespace dream::world
