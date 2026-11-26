#include "common/headless_host.hpp"

#include <dream/runtime/engine_runtime.hpp>

#include <array>
#include <iostream>

int main() {
    using namespace dream;
    examples::HeadlessHost host;
    runtime::EngineRuntime engine(host, host);
    if (!engine.initialize({1U})) {
        std::cerr << "Could not initialize 3D sample runtime\n";
        return 1;
    }

    physics3d::BodyDescription3D floor;
    floor.type = physics3d::BodyType3D::Static;
    floor.position = {0.0F, -0.5F, 0.0F};
    floor.collider = physics3d::BoxCollider3D{{}, {5.0F, 0.5F, 5.0F}, {}};
    if (!engine.physics3d().create_body(floor)) return 2;

    physics3d::BodyDescription3D ball;
    ball.type = physics3d::BodyType3D::Dynamic;
    ball.position = {0.0F, 2.0F, 0.0F};
    ball.collider = physics3d::SphereCollider3D{{}, 0.5F};
    const auto ball_body = engine.physics3d().create_body(ball);
    if (!ball_body) return 3;

    constexpr std::array<render::Vertex3D, 3> vertices{{
        {{-0.8F, -0.6F, 0.0F}, {0.0F, 0.0F, 1.0F}, {0.0F, 1.0F}},
        {{ 0.8F, -0.6F, 0.0F}, {0.0F, 0.0F, 1.0F}, {1.0F, 1.0F}},
        {{ 0.0F,  0.8F, 0.0F}, {0.0F, 0.0F, 1.0F}, {0.5F, 0.0F}}
    }};
    constexpr std::array<std::uint32_t, 3> indices{0U, 1U, 2U};
    const render::MeshDescription mesh{vertices, indices};
    if (!engine.render_device().create_mesh(1U, mesh)) return 4;

    render::MaterialDescription material;
    material.base_color = {0.25F, 0.85F, 0.45F, 1.0F};
    if (!engine.render_device().create_material(1U, material)) return 5;

    ui::UiNodeDescription panel;
    panel.type = ui::UiNodeType::Panel;
    panel.style.width = ui::UiSize::fixed(180.0F);
    panel.style.height = ui::UiSize::fixed(42.0F);
    panel.style.background = {0.04F, 0.06F, 0.1F, 0.8F};
    const auto panel_handle = engine.ui().create_node({}, panel);
    examples::BasicUiMetrics ui_metrics;
    if (!panel_handle ||
        engine.ui().layout({0, 0, 800, 600}, ui_metrics) != ui::UiError::None)
        return 6;

    world::Camera3D camera3d;
    camera3d.position = {0.0F, 2.0F, 6.0F};
    world::CameraMatrices3D camera3d_matrices;
    if (world::build_camera_matrices(camera3d, 800U, 600U, camera3d_matrices) !=
        world::CameraBuildError::None)
        return 7;

    world::Camera2D ui_camera;
    world::CameraMatrices2D ui_camera_matrices;
    if (world::build_camera_matrices(ui_camera, 800U, 600U, ui_camera_matrices) !=
        world::CameraBuildError::None)
        return 8;

    for (unsigned frame = 0; frame < 180U; ++frame) {
        const auto update = engine.update(1.0 / 60.0);
        if (!update) return 9;
        if (!engine.begin_render_frame({0.025F, 0.04F, 0.075F, 1.0F})) return 10;
        if (engine.frame_builder().set_camera_3d(camera3d_matrices) !=
                render::FrameBuilderError::None ||
            engine.frame_builder().set_camera_2d(ui_camera_matrices) !=
                render::FrameBuilderError::None)
            return 11;

        const auto state = engine.physics3d().body_state(ball_body.handle);
        if (!state) return 12;
        world::Transform3D transform;
        transform.position = {0.0F, 0.0F, 0.0F};
        transform.scale = {0.8F, 0.8F, 0.8F};
        render::DrawCommand3D draw;
        draw.mesh_id = 1U;
        draw.material_id = 1U;
        draw.model = world::transform_matrix(transform);
        draw.tint = {1.0F, 1.0F, 1.0F, 1.0F};
        if (engine.frame_builder().add_mesh(draw) !=
            render::FrameBuilderError::None)
            return 13;

        if (engine.enqueue_ui_draws(ui_camera, {0, 0, 800, 600}) !=
            ui::UiRenderError::None)
            return 14;
        if (!engine.render_frame()) return 15;
    }

    const auto final_ball = engine.physics3d().body_state(ball_body.handle);
    const bool settled = final_ball && final_ball->position.y > 0.3F &&
                         final_ball->position.y < 0.9F;
    const bool drew_3d_and_ui = host.frames == 180U &&
        host.last_3d_draws == 1U && host.last_2d_draws == 1U;
    engine.shutdown();
    if (!settled || !drew_3d_and_ui || host.uploaded_meshes != 0U) {
        std::cerr << "3D sample failed physics or render integration checks\n";
        return 16;
    }
    std::cout << "3D sample passed: 180 fixed-step frames, 3D mesh and UI rendered.\n";
    return 0;
}
