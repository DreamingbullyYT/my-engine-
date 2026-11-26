#pragma once

#include <dream/world/transform.hpp>

#include <cmath>
#include <cstdint>
#include <limits>

namespace dream::world {

enum class CameraProjection : std::uint8_t {
    Perspective,
    Orthographic
};

enum class CameraBuildError : std::uint8_t {
    None,
    InvalidViewport,
    InvalidPosition,
    InvalidRotation,
    InvalidProjection
};

struct Camera2D final {
    Vec2 position{};
    float rotation_radians{0.0F};
    float orthographic_height{10.0F};
    float zoom{1.0F};
};

struct Camera3D final {
    Vec3 position{};
    Quaternion rotation{};
    CameraProjection projection{CameraProjection::Perspective};
    float vertical_fov_radians{1.0471975512F};
    float orthographic_height{10.0F};
    float near_plane{0.1F};
    float far_plane{1000.0F};
};

struct CameraMatrices2D final {
    Matrix3x3 view{};
    Matrix3x3 projection{};
    Matrix3x3 view_projection{};
};

struct CameraMatrices3D final {
    Matrix4x4 view{};
    Matrix4x4 projection{};
    Matrix4x4 view_projection{};
};

namespace detail {

[[nodiscard]] inline Vec3 rotate_by_quaternion(
    Vec3 value, Quaternion rotation) noexcept {
    const Vec3 axis{rotation.x, rotation.y, rotation.z};
    const Vec3 first{
        axis.y * value.z - axis.z * value.y,
        axis.z * value.x - axis.x * value.z,
        axis.x * value.y - axis.y * value.x
    };
    const Vec3 second{
        axis.y * first.z - axis.z * first.y,
        axis.z * first.x - axis.x * first.z,
        axis.x * first.y - axis.y * first.x
    };
    return {
        value.x + 2.0F * (rotation.w * first.x + second.x),
        value.y + 2.0F * (rotation.w * first.y + second.y),
        value.z + 2.0F * (rotation.w * first.z + second.z)
    };
}

} // namespace detail

// The engine camera convention is right-handed with camera forward along -Z.
// Projection depth is [0, 1], matching Vulkan/Direct3D. Backends using a
// different clip-space convention must convert at the backend boundary.
[[nodiscard]] inline CameraBuildError build_camera_matrices(
    const Camera2D& camera, std::uint32_t viewport_width,
    std::uint32_t viewport_height, CameraMatrices2D& output) noexcept {
    if (viewport_width == 0U || viewport_height == 0U)
        return CameraBuildError::InvalidViewport;

    if (!std::isfinite(camera.position.x) ||
        !std::isfinite(camera.position.y) ||
        !std::isfinite(camera.rotation_radians)) {
        return CameraBuildError::InvalidPosition;
    }
    if (!std::isfinite(camera.orthographic_height) ||
        !std::isfinite(camera.zoom) ||
        camera.orthographic_height <= 0.0F || camera.zoom <= 0.0F) {
        return CameraBuildError::InvalidProjection;
    }

    const float world_height = camera.orthographic_height / camera.zoom;
    const float aspect = static_cast<float>(viewport_width) /
                         static_cast<float>(viewport_height);
    const float world_width = world_height * aspect;
    if (!std::isfinite(world_height) || !std::isfinite(world_width) ||
        world_height <= 0.0F || world_width <= 0.0F) {
        return CameraBuildError::InvalidProjection;
    }

    const float cosine = std::cos(camera.rotation_radians);
    const float sine = std::sin(camera.rotation_radians);

    CameraMatrices2D result;
    result.view.at(0, 0) = cosine;
    result.view.at(0, 1) = sine;
    result.view.at(1, 0) = -sine;
    result.view.at(1, 1) = cosine;
    result.view.at(0, 2) =
        -(cosine * camera.position.x + sine * camera.position.y);
    result.view.at(1, 2) =
        -(-sine * camera.position.x + cosine * camera.position.y);

    result.projection.at(0, 0) = 2.0F / world_width;
    result.projection.at(1, 1) = 2.0F / world_height;
    result.view_projection = multiply(result.projection, result.view);
    output = result;
    return CameraBuildError::None;
}

[[nodiscard]] inline CameraBuildError build_camera_matrices(
    const Camera3D& camera, std::uint32_t viewport_width,
    std::uint32_t viewport_height, CameraMatrices3D& output) noexcept {
    if (viewport_width == 0U || viewport_height == 0U)
        return CameraBuildError::InvalidViewport;

    if (!std::isfinite(camera.position.x) ||
        !std::isfinite(camera.position.y) ||
        !std::isfinite(camera.position.z)) {
        return CameraBuildError::InvalidPosition;
    }

    const Quaternion rotation = camera.rotation;
    if (!std::isfinite(rotation.x) || !std::isfinite(rotation.y) ||
        !std::isfinite(rotation.z) || !std::isfinite(rotation.w)) {
        return CameraBuildError::InvalidRotation;
    }

    const float quaternion_length_squared =
        rotation.x * rotation.x + rotation.y * rotation.y +
        rotation.z * rotation.z + rotation.w * rotation.w;
    if (!std::isfinite(quaternion_length_squared) ||
        quaternion_length_squared < 1.0e-12F) {
        return CameraBuildError::InvalidRotation;
    }

    if (!std::isfinite(camera.near_plane) ||
        !std::isfinite(camera.far_plane) ||
        camera.near_plane <= 0.0F ||
        camera.far_plane <= camera.near_plane) {
        return CameraBuildError::InvalidProjection;
    }

    const float aspect = static_cast<float>(viewport_width) /
                         static_cast<float>(viewport_height);
    if (!std::isfinite(aspect) || aspect <= 0.0F)
        return CameraBuildError::InvalidProjection;

    Quaternion inverse_rotation{
        -rotation.x, -rotation.y, -rotation.z, rotation.w
    };
    const float inverse_length = 1.0F / std::sqrt(quaternion_length_squared);
    inverse_rotation.x *= inverse_length;
    inverse_rotation.y *= inverse_length;
    inverse_rotation.z *= inverse_length;
    inverse_rotation.w *= inverse_length;

    const Vec3 right = detail::rotate_by_quaternion(Vec3{1.0F, 0.0F, 0.0F}, inverse_rotation);
    const Vec3 up = detail::rotate_by_quaternion(Vec3{0.0F, 1.0F, 0.0F}, inverse_rotation);
    const Vec3 backward = detail::rotate_by_quaternion(Vec3{0.0F, 0.0F, 1.0F}, inverse_rotation);
    const Vec3 negative_position{
        -camera.position.x, -camera.position.y, -camera.position.z
    };
    const Vec3 translated = detail::rotate_by_quaternion(negative_position, inverse_rotation);

    CameraMatrices3D result;
    result.view.at(0, 0) = right.x;
    result.view.at(0, 1) = up.x;
    result.view.at(0, 2) = backward.x;
    result.view.at(1, 0) = right.y;
    result.view.at(1, 1) = up.y;
    result.view.at(1, 2) = backward.y;
    result.view.at(2, 0) = right.z;
    result.view.at(2, 1) = up.z;
    result.view.at(2, 2) = backward.z;
    result.view.at(0, 3) = translated.x;
    result.view.at(1, 3) = translated.y;
    result.view.at(2, 3) = translated.z;

    if (camera.projection == CameraProjection::Perspective) {
        constexpr float pi = 3.14159265358979323846F;
        if (!std::isfinite(camera.vertical_fov_radians) ||
            camera.vertical_fov_radians <= 0.001F ||
            camera.vertical_fov_radians >= pi - 0.001F) {
            return CameraBuildError::InvalidProjection;
        }

        const float tangent = std::tan(camera.vertical_fov_radians * 0.5F);
        if (!std::isfinite(tangent) || tangent <= 0.0F)
            return CameraBuildError::InvalidProjection;
        const float focal = 1.0F / tangent;
        result.projection.at(0, 0) = focal / aspect;
        result.projection.at(1, 1) = focal;
        result.projection.at(2, 2) =
            camera.far_plane / (camera.near_plane - camera.far_plane);
        result.projection.at(2, 3) =
            (camera.far_plane * camera.near_plane) /
            (camera.near_plane - camera.far_plane);
        result.projection.at(3, 2) = -1.0F;
        result.projection.at(3, 3) = 0.0F;
    } else if (camera.projection == CameraProjection::Orthographic) {
        if (!std::isfinite(camera.orthographic_height) ||
            camera.orthographic_height <= 0.0F) {
            return CameraBuildError::InvalidProjection;
        }

        const float half_height = camera.orthographic_height * 0.5F;
        const float half_width = half_height * aspect;
        if (!std::isfinite(half_height) || !std::isfinite(half_width) ||
            half_height <= 0.0F || half_width <= 0.0F) {
            return CameraBuildError::InvalidProjection;
        }

        result.projection.at(0, 0) = 1.0F / half_width;
        result.projection.at(1, 1) = 1.0F / half_height;
        result.projection.at(2, 2) =
            1.0F / (camera.near_plane - camera.far_plane);
        result.projection.at(2, 3) =
            camera.near_plane / (camera.near_plane - camera.far_plane);
    } else {
        return CameraBuildError::InvalidProjection;
    }

    result.view_projection = multiply(result.projection, result.view);
    output = result;
    return CameraBuildError::None;
}

} // namespace dream::world
