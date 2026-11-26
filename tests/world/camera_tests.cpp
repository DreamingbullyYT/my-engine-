#include <dream/world/camera.hpp>

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string_view>

namespace {
using namespace dream::world;

void require(bool condition, std::string_view message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

bool near(float left, float right, float epsilon = 0.0005F) {
    return std::abs(left - right) <= epsilon;
}

Vec3 transform_clip(const Matrix4x4& matrix, Vec3 point, float& w) {
    const float x = matrix.at(0, 0) * point.x +
                    matrix.at(0, 1) * point.y +
                    matrix.at(0, 2) * point.z + matrix.at(0, 3);
    const float y = matrix.at(1, 0) * point.x +
                    matrix.at(1, 1) * point.y +
                    matrix.at(1, 2) * point.z + matrix.at(1, 3);
    const float z = matrix.at(2, 0) * point.x +
                    matrix.at(2, 1) * point.y +
                    matrix.at(2, 2) * point.z + matrix.at(2, 3);
    w = matrix.at(3, 0) * point.x +
        matrix.at(3, 1) * point.y +
        matrix.at(3, 2) * point.z + matrix.at(3, 3);
    return {x, y, z};
}
}

int main() {
    CameraMatrices2D matrices2d;
    Camera2D camera2d;
    camera2d.position = {10.0F, 20.0F};
    camera2d.rotation_radians = 1.57079632679F;
    camera2d.orthographic_height = 10.0F;
    camera2d.zoom = 2.0F;
    require(build_camera_matrices(camera2d, 800U, 600U, matrices2d) ==
                CameraBuildError::None,
            "valid 2D camera builds");

    const Vec2 camera_center = transform_point(
        matrices2d.view_projection, camera2d.position);
    require(near(camera_center.x, 0.0F) && near(camera_center.y, 0.0F),
            "2D camera position maps to view center");

    const float world_height = camera2d.orthographic_height / camera2d.zoom;
    const float world_width = world_height * (800.0F / 600.0F);
    const Vec2 right_edge = transform_point(
        matrices2d.view_projection,
        {camera2d.position.x, camera2d.position.y + world_width * 0.5F});
    require(near(right_edge.x, 1.0F),
            "2D projection respects aspect ratio and rotation");

    Camera2D no_viewport;
    require(build_camera_matrices(no_viewport, 0U, 480U, matrices2d) ==
                CameraBuildError::InvalidViewport,
            "zero width viewport is rejected");
    Camera2D invalid_zoom;
    invalid_zoom.zoom = 0.0F;
    require(build_camera_matrices(invalid_zoom, 640U, 480U, matrices2d) ==
                CameraBuildError::InvalidProjection,
            "zero camera zoom is rejected");

    Camera3D camera3d;
    camera3d.position = {0.0F, 0.0F, 5.0F};
    CameraMatrices3D matrices3d;
    require(build_camera_matrices(camera3d, 1280U, 720U, matrices3d) ==
                CameraBuildError::None,
            "valid perspective camera builds");
    require(matrices3d.projection.at(3, 3) == 0.0F &&
            matrices3d.projection.at(3, 2) == -1.0F,
            "perspective homogeneous w is explicitly -view-z");

    const Vec3 view_origin = transform_point(matrices3d.view, camera3d.position);
    require(near(view_origin.x, 0.0F) && near(view_origin.y, 0.0F) &&
            near(view_origin.z, 0.0F),
            "camera position maps to view origin");

    float clip_w = 0.0F;
    const Vec3 near_clip = transform_clip(
        matrices3d.projection, {0.0F, 0.0F, -camera3d.near_plane}, clip_w);
    require(near(near_clip.z / clip_w, 0.0F),
            "perspective near plane maps to zero depth");

    const Vec3 far_clip = transform_clip(
        matrices3d.projection, {0.0F, 0.0F, -camera3d.far_plane}, clip_w);
    require(near(far_clip.z / clip_w, 1.0F),
            "perspective far plane maps to one depth");

    const float half_y = std::tan(camera3d.vertical_fov_radians * 0.5F);
    const float half_x = half_y * (1280.0F / 720.0F);
    const Vec3 right_edge_clip = transform_clip(
        matrices3d.projection, {half_x, 0.0F, -1.0F}, clip_w);
    require(near(right_edge_clip.x / clip_w, 1.0F),
            "perspective projection respects horizontal field of view");

    Camera3D yawed;
    const float half_yaw = 0.78539816339F;
    yawed.rotation = {0.0F, std::sin(half_yaw), 0.0F, std::cos(half_yaw)};
    require(build_camera_matrices(yawed, 800U, 600U, matrices3d) ==
                CameraBuildError::None,
            "rotated camera builds");
    const Vec3 forward_world{-5.0F, 0.0F, 0.0F};
    const Vec3 forward_view = transform_point(matrices3d.view, forward_world);
    require(near(forward_view.x, 0.0F) && near(forward_view.y, 0.0F) &&
            near(forward_view.z, -5.0F),
            "camera orientation transforms world forward into view -Z");

    Camera3D orthographic;
    orthographic.projection = CameraProjection::Orthographic;
    orthographic.orthographic_height = 20.0F;
    require(build_camera_matrices(orthographic, 1000U, 500U, matrices3d) ==
                CameraBuildError::None,
            "orthographic camera builds");
    const float half_width = 20.0F;
    const Vec3 ortho_edge = transform_clip(
        matrices3d.projection, {half_width, 0.0F, -1.0F}, clip_w);
    require(near(ortho_edge.x, 1.0F),
            "orthographic projection uses vertical size and aspect ratio");

    const Vec3 ortho_near = transform_clip(
        matrices3d.projection, {0.0F, 0.0F, -orthographic.near_plane}, clip_w);
    require(near(ortho_near.z, 0.0F),
            "orthographic near plane maps to zero depth");
    const Vec3 ortho_far = transform_clip(
        matrices3d.projection, {0.0F, 0.0F, -orthographic.far_plane}, clip_w);
    require(near(ortho_far.z, 1.0F),
            "orthographic far plane maps to one depth");

    Camera3D invalid_quaternion;
    invalid_quaternion.rotation = {0, 0, 0, 0};
    require(build_camera_matrices(invalid_quaternion, 640U, 480U, matrices3d) ==
                CameraBuildError::InvalidRotation,
            "zero quaternion is rejected");

    Camera3D invalid_projection;
    invalid_projection.far_plane = invalid_projection.near_plane;
    require(build_camera_matrices(invalid_projection, 640U, 480U, matrices3d) ==
                CameraBuildError::InvalidProjection,
            "invalid clipping planes are rejected");

    Camera3D invalid_fov;
    invalid_fov.vertical_fov_radians = std::numeric_limits<float>::quiet_NaN();
    require(build_camera_matrices(invalid_fov, 640U, 480U, matrices3d) ==
                CameraBuildError::InvalidProjection,
            "non-finite field of view is rejected");

    std::cout << "Dream Engine camera tests passed.\n";
    return EXIT_SUCCESS;
}
