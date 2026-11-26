#pragma once

#include <dream/world/world.hpp>

#include <array>
#include <cmath>
#include <cstddef>

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

// Matrices use column-major storage and column vectors. Matrix composition is
// used instead of decomposing back to TRS, preserving shear from hierarchical
// combinations such as a rotated child under a non-uniformly scaled parent.
struct Matrix3x3 final {
    std::array<float, 9> values{
        1.0F, 0.0F, 0.0F,
        0.0F, 1.0F, 0.0F,
        0.0F, 0.0F, 1.0F
    };

    [[nodiscard]] constexpr float& at(std::size_t row, std::size_t column) noexcept {
        return values[column * 3U + row];
    }
    [[nodiscard]] constexpr float at(std::size_t row, std::size_t column) const noexcept {
        return values[column * 3U + row];
    }
};

struct Matrix4x4 final {
    std::array<float, 16> values{
        1.0F, 0.0F, 0.0F, 0.0F,
        0.0F, 1.0F, 0.0F, 0.0F,
        0.0F, 0.0F, 1.0F, 0.0F,
        0.0F, 0.0F, 0.0F, 1.0F
    };

    [[nodiscard]] constexpr float& at(std::size_t row, std::size_t column) noexcept {
        return values[column * 4U + row];
    }
    [[nodiscard]] constexpr float at(std::size_t row, std::size_t column) const noexcept {
        return values[column * 4U + row];
    }
};

struct Parent final {
    Entity entity{};

    [[nodiscard]] constexpr bool has_parent() const noexcept {
        return static_cast<bool>(entity);
    }
};

[[nodiscard]] inline Matrix3x3 transform_matrix(const Transform2D& transform) noexcept {
    const float cosine = std::cos(transform.rotation_radians);
    const float sine = std::sin(transform.rotation_radians);
    Matrix3x3 result;
    result.at(0, 0) = cosine * transform.scale.x;
    result.at(1, 0) = sine * transform.scale.x;
    result.at(0, 1) = -sine * transform.scale.y;
    result.at(1, 1) = cosine * transform.scale.y;
    result.at(0, 2) = transform.position.x;
    result.at(1, 2) = transform.position.y;
    return result;
}

[[nodiscard]] inline Matrix4x4 transform_matrix(const Transform3D& transform) noexcept {
    const auto q = transform.rotation;
    const float xx = q.x * q.x;
    const float yy = q.y * q.y;
    const float zz = q.z * q.z;
    const float xy = q.x * q.y;
    const float xz = q.x * q.z;
    const float yz = q.y * q.z;
    const float wx = q.w * q.x;
    const float wy = q.w * q.y;
    const float wz = q.w * q.z;

    Matrix4x4 result;
    result.at(0, 0) = (1.0F - 2.0F * (yy + zz)) * transform.scale.x;
    result.at(1, 0) = (2.0F * (xy + wz)) * transform.scale.x;
    result.at(2, 0) = (2.0F * (xz - wy)) * transform.scale.x;

    result.at(0, 1) = (2.0F * (xy - wz)) * transform.scale.y;
    result.at(1, 1) = (1.0F - 2.0F * (xx + zz)) * transform.scale.y;
    result.at(2, 1) = (2.0F * (yz + wx)) * transform.scale.y;

    result.at(0, 2) = (2.0F * (xz + wy)) * transform.scale.z;
    result.at(1, 2) = (2.0F * (yz - wx)) * transform.scale.z;
    result.at(2, 2) = (1.0F - 2.0F * (xx + yy)) * transform.scale.z;

    result.at(0, 3) = transform.position.x;
    result.at(1, 3) = transform.position.y;
    result.at(2, 3) = transform.position.z;
    return result;
}

[[nodiscard]] inline Matrix3x3 multiply(
    const Matrix3x3& left, const Matrix3x3& right) noexcept {
    Matrix3x3 result;
    for (std::size_t column = 0; column < 3; ++column) {
        for (std::size_t row = 0; row < 3; ++row) {
            result.at(row, column) = 0.0F;
            for (std::size_t index = 0; index < 3; ++index)
                result.at(row, column) += left.at(row, index) * right.at(index, column);
        }
    }
    return result;
}

[[nodiscard]] inline Matrix4x4 multiply(
    const Matrix4x4& left, const Matrix4x4& right) noexcept {
    Matrix4x4 result;
    for (std::size_t column = 0; column < 4; ++column) {
        for (std::size_t row = 0; row < 4; ++row) {
            result.at(row, column) = 0.0F;
            for (std::size_t index = 0; index < 4; ++index)
                result.at(row, column) += left.at(row, index) * right.at(index, column);
        }
    }
    return result;
}

[[nodiscard]] inline Vec2 transform_point(
    const Matrix3x3& matrix, Vec2 point) noexcept {
    return {
        matrix.at(0, 0) * point.x + matrix.at(0, 1) * point.y + matrix.at(0, 2),
        matrix.at(1, 0) * point.x + matrix.at(1, 1) * point.y + matrix.at(1, 2)
    };
}

[[nodiscard]] inline Vec3 transform_point(
    const Matrix4x4& matrix, Vec3 point) noexcept {
    return {
        matrix.at(0, 0) * point.x + matrix.at(0, 1) * point.y +
            matrix.at(0, 2) * point.z + matrix.at(0, 3),
        matrix.at(1, 0) * point.x + matrix.at(1, 1) * point.y +
            matrix.at(1, 2) * point.z + matrix.at(1, 3),
        matrix.at(2, 0) * point.x + matrix.at(2, 1) * point.y +
            matrix.at(2, 2) * point.z + matrix.at(2, 3)
    };
}

} // namespace dream::world
