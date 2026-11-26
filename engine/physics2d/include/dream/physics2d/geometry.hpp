#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <optional>

namespace dream::physics2d {

struct Vec2 final {
    float x{0.0F};
    float y{0.0F};
};

[[nodiscard]] constexpr Vec2 operator+(Vec2 left, Vec2 right) noexcept {
    return {left.x + right.x, left.y + right.y};
}
[[nodiscard]] constexpr Vec2 operator-(Vec2 left, Vec2 right) noexcept {
    return {left.x - right.x, left.y - right.y};
}
[[nodiscard]] constexpr Vec2 operator*(Vec2 value, float scalar) noexcept {
    return {value.x * scalar, value.y * scalar};
}
[[nodiscard]] constexpr Vec2 operator/(Vec2 value, float scalar) noexcept {
    return {value.x / scalar, value.y / scalar};
}
[[nodiscard]] constexpr float dot(Vec2 left, Vec2 right) noexcept {
    return left.x * right.x + left.y * right.y;
}
[[nodiscard]] constexpr float cross(Vec2 left, Vec2 right) noexcept {
    return left.x * right.y - left.y * right.x;
}
[[nodiscard]] inline float length_squared(Vec2 value) noexcept {
    return dot(value, value);
}
[[nodiscard]] inline float length(Vec2 value) noexcept {
    return std::sqrt(length_squared(value));
}
[[nodiscard]] inline Vec2 normalized(Vec2 value) noexcept {
    const float magnitude = length(value);
    return magnitude > 1.0e-12F ? value / magnitude : Vec2{1.0F, 0.0F};
}
[[nodiscard]] inline Vec2 rotate(Vec2 value, float radians) noexcept {
    const float cosine = std::cos(radians);
    const float sine = std::sin(radians);
    return {value.x * cosine - value.y * sine,
            value.x * sine + value.y * cosine};
}

struct Aabb final {
    Vec2 minimum{};
    Vec2 maximum{};

    [[nodiscard]] static Aabb from_center_half_extents(
        Vec2 center, Vec2 half_extents) noexcept {
        return {
            {center.x - half_extents.x, center.y - half_extents.y},
            {center.x + half_extents.x, center.y + half_extents.y}
        };
    }

    [[nodiscard]] Vec2 center() const noexcept {
        return (minimum + maximum) * 0.5F;
    }

    [[nodiscard]] Vec2 half_extents() const noexcept {
        return (maximum - minimum) * 0.5F;
    }

    [[nodiscard]] bool valid() const noexcept {
        return std::isfinite(minimum.x) && std::isfinite(minimum.y) &&
               std::isfinite(maximum.x) && std::isfinite(maximum.y) &&
               minimum.x <= maximum.x && minimum.y <= maximum.y;
    }
};

struct Circle final {
    Vec2 center{};
    float radius{0.5F};

    [[nodiscard]] bool valid() const noexcept {
        return std::isfinite(center.x) && std::isfinite(center.y) &&
               std::isfinite(radius) && radius >= 0.0F;
    }

    [[nodiscard]] Aabb bounds() const noexcept {
        const Vec2 extent{radius, radius};
        return {center - extent, center + extent};
    }
};

struct OrientedBox final {
    Vec2 center{};
    Vec2 half_extents{0.5F, 0.5F};
    float rotation_radians{0.0F};

    [[nodiscard]] bool valid() const noexcept {
        return std::isfinite(center.x) && std::isfinite(center.y) &&
               std::isfinite(half_extents.x) &&
               std::isfinite(half_extents.y) &&
               std::isfinite(rotation_radians) &&
               half_extents.x >= 0.0F && half_extents.y >= 0.0F;
    }

    [[nodiscard]] std::array<Vec2, 2> axes() const noexcept {
        const float cosine = std::cos(rotation_radians);
        const float sine = std::sin(rotation_radians);
        return {{{cosine, sine}, {-sine, cosine}}};
    }

    [[nodiscard]] std::array<Vec2, 4> vertices() const noexcept {
        const Vec2 x_axis = axes()[0] * half_extents.x;
        const Vec2 y_axis = axes()[1] * half_extents.y;
        return {
            center - x_axis - y_axis,
            center + x_axis - y_axis,
            center + x_axis + y_axis,
            center - x_axis + y_axis
        };
    }

    [[nodiscard]] Aabb bounds() const noexcept {
        const auto points = vertices();
        Vec2 minimum = points[0];
        Vec2 maximum = points[0];
        for (const auto point : points) {
            minimum.x = std::min(minimum.x, point.x);
            minimum.y = std::min(minimum.y, point.y);
            maximum.x = std::max(maximum.x, point.x);
            maximum.y = std::max(maximum.y, point.y);
        }
        return {minimum, maximum};
    }
};

struct ContactManifold final {
    // Points from the first shape toward the second. For a usual separated
    // pair, moving shape A opposite this normal resolves penetration.
    Vec2 normal{1.0F, 0.0F};
    Vec2 point{};
    float penetration{0.0F};
};

[[nodiscard]] inline ContactManifold flipped(ContactManifold manifold) noexcept {
    manifold.normal = manifold.normal * -1.0F;
    return manifold;
}

[[nodiscard]] inline bool intersects(const Aabb& left, const Aabb& right) noexcept {
    if (!left.valid() || !right.valid()) return false;
    return left.minimum.x <= right.maximum.x &&
           left.maximum.x >= right.minimum.x &&
           left.minimum.y <= right.maximum.y &&
           left.maximum.y >= right.minimum.y;
}

[[nodiscard]] inline std::optional<ContactManifold> collide(
    const Aabb& left, const Aabb& right) noexcept {
    if (!left.valid() || !right.valid()) return std::nullopt;

    const float overlap_x =
        std::min(left.maximum.x, right.maximum.x) -
        std::max(left.minimum.x, right.minimum.x);
    const float overlap_y =
        std::min(left.maximum.y, right.maximum.y) -
        std::max(left.minimum.y, right.minimum.y);
    if (overlap_x < 0.0F || overlap_y < 0.0F) return std::nullopt;

    const Vec2 left_center = left.center();
    const Vec2 right_center = right.center();
    ContactManifold manifold;
    if (overlap_x <= overlap_y) {
        const float direction = right_center.x >= left_center.x ? 1.0F : -1.0F;
        manifold.normal = {direction, 0.0F};
        manifold.penetration = overlap_x;
        const float contact_y =
            (std::max(left.minimum.y, right.minimum.y) +
             std::min(left.maximum.y, right.maximum.y)) * 0.5F;
        const float edge_x = direction > 0.0F
            ? (left.maximum.x + right.minimum.x) * 0.5F
            : (left.minimum.x + right.maximum.x) * 0.5F;
        manifold.point = {edge_x, contact_y};
    } else {
        const float direction = right_center.y >= left_center.y ? 1.0F : -1.0F;
        manifold.normal = {0.0F, direction};
        manifold.penetration = overlap_y;
        const float contact_x =
            (std::max(left.minimum.x, right.minimum.x) +
             std::min(left.maximum.x, right.maximum.x)) * 0.5F;
        const float edge_y = direction > 0.0F
            ? (left.maximum.y + right.minimum.y) * 0.5F
            : (left.minimum.y + right.maximum.y) * 0.5F;
        manifold.point = {contact_x, edge_y};
    }
    return manifold;
}

[[nodiscard]] inline std::optional<ContactManifold> collide(
    const Circle& left, const Circle& right) noexcept {
    if (!left.valid() || !right.valid()) return std::nullopt;

    const Vec2 delta = right.center - left.center;
    const float distance_squared = length_squared(delta);
    const float radii = left.radius + right.radius;
    if (distance_squared > radii * radii) return std::nullopt;

    const float distance = std::sqrt(std::max(0.0F, distance_squared));
    const Vec2 normal = distance > 1.0e-12F
        ? delta / distance : Vec2{1.0F, 0.0F};
    const Vec2 point_a = left.center + normal * left.radius;
    const Vec2 point_b = right.center - normal * right.radius;
    return ContactManifold{
        normal, (point_a + point_b) * 0.5F,
        std::max(0.0F, radii - distance)
    };
}

[[nodiscard]] inline std::optional<ContactManifold> collide(
    const Circle& circle, const Aabb& box) noexcept {
    if (!circle.valid() || !box.valid()) return std::nullopt;

    const Vec2 closest{
        std::clamp(circle.center.x, box.minimum.x, box.maximum.x),
        std::clamp(circle.center.y, box.minimum.y, box.maximum.y)
    };
    const Vec2 delta = closest - circle.center;
    const float distance_squared = length_squared(delta);
    if (distance_squared > circle.radius * circle.radius) return std::nullopt;

    if (distance_squared > 1.0e-12F) {
        const float distance = std::sqrt(distance_squared);
        return ContactManifold{
            delta / distance, closest,
            std::max(0.0F, circle.radius - distance)
        };
    }

    // The circle centre lies inside or exactly on the box. Choose the nearest
    // exit face. The normal is reversed so moving the first shape opposite it
    // takes the circle through that face instead of deeper into the box.
    const std::array<float, 4> distances{
        circle.center.x - box.minimum.x,
        box.maximum.x - circle.center.x,
        circle.center.y - box.minimum.y,
        box.maximum.y - circle.center.y
    };
    const auto nearest = static_cast<std::size_t>(
        std::min_element(distances.begin(), distances.end()) - distances.begin());

    Vec2 outward{};
    Vec2 point{};
    switch (nearest) {
    case 0:
        outward = {-1.0F, 0.0F};
        point = {box.minimum.x, circle.center.y};
        break;
    case 1:
        outward = {1.0F, 0.0F};
        point = {box.maximum.x, circle.center.y};
        break;
    case 2:
        outward = {0.0F, -1.0F};
        point = {circle.center.x, box.minimum.y};
        break;
    default:
        outward = {0.0F, 1.0F};
        point = {circle.center.x, box.maximum.y};
        break;
    }

    return ContactManifold{
        outward * -1.0F, point,
        circle.radius + distances[nearest]
    };
}

[[nodiscard]] inline std::optional<ContactManifold> collide(
    const Aabb& box, const Circle& circle) noexcept {
    auto result = collide(circle, box);
    if (result) *result = flipped(*result);
    return result;
}

[[nodiscard]] inline std::optional<ContactManifold> collide(
    const Circle& circle, const OrientedBox& box) noexcept {
    if (!circle.valid() || !box.valid()) return std::nullopt;
    const Vec2 local_center = rotate(
        circle.center - box.center, -box.rotation_radians);
    const Aabb local_box{
        {-box.half_extents.x, -box.half_extents.y},
        {box.half_extents.x, box.half_extents.y}
    };
    auto result = collide(Circle{local_center, circle.radius}, local_box);
    if (!result) return std::nullopt;
    result->normal = rotate(result->normal, box.rotation_radians);
    result->point = rotate(result->point, box.rotation_radians) + box.center;
    return result;
}

[[nodiscard]] inline std::optional<ContactManifold> collide(
    const OrientedBox& box, const Circle& circle) noexcept {
    auto result = collide(circle, box);
    if (result) *result = flipped(*result);
    return result;
}

[[nodiscard]] inline std::optional<ContactManifold> collide(
    const OrientedBox& left, const OrientedBox& right) noexcept {
    if (!left.valid() || !right.valid()) return std::nullopt;
    const auto left_axes = left.axes();
    const auto right_axes = right.axes();
    const std::array<Vec2, 4> axes{
        left_axes[0], left_axes[1], right_axes[0], right_axes[1]
    };

    const Vec2 center_delta = right.center - left.center;
    float minimum_overlap = std::numeric_limits<float>::max();
    Vec2 minimum_axis{1.0F, 0.0F};

    for (const Vec2 axis : axes) {
        const float left_radius =
            left.half_extents.x * std::abs(dot(axis, left_axes[0])) +
            left.half_extents.y * std::abs(dot(axis, left_axes[1]));
        const float right_radius =
            right.half_extents.x * std::abs(dot(axis, right_axes[0])) +
            right.half_extents.y * std::abs(dot(axis, right_axes[1]));
        const float signed_distance = dot(center_delta, axis);
        const float overlap =
            left_radius + right_radius - std::abs(signed_distance);
        if (overlap < 0.0F) return std::nullopt;
        if (overlap < minimum_overlap) {
            minimum_overlap = overlap;
            minimum_axis = signed_distance >= 0.0F ? axis : axis * -1.0F;
        }
    }

    const auto left_vertices = left.vertices();
    const auto right_vertices = right.vertices();
    const auto support = [](const auto& vertices, Vec2 direction) {
        Vec2 best = vertices[0];
        float best_projection = dot(best, direction);
        for (const auto vertex : vertices) {
            const float projection = dot(vertex, direction);
            if (projection > best_projection) {
                best = vertex;
                best_projection = projection;
            }
        }
        return best;
    };

    const Vec2 point_a = support(left_vertices, minimum_axis);
    const Vec2 point_b = support(right_vertices, minimum_axis * -1.0F);
    return ContactManifold{
        minimum_axis, (point_a + point_b) * 0.5F,
        minimum_overlap
    };
}

} // namespace dream::physics2d
