#include "common/headless_host.hpp"

#include <dream/runtime/engine_runtime.hpp>

#include <cmath>
#include <iostream>

int main() {
    using namespace dream;
    examples::HeadlessHost host;
    runtime::EngineRuntime engine(host, host);
    if (!engine.initialize({1U})) {
        std::cerr << "Could not initialize 2D sample runtime\n";
        return 1;
    }

    physics2d::BodyDescription2D ground;
    ground.type = physics2d::BodyType::Static;
    ground.position = {0.0F, -2.0F};
    ground.collider = physics2d::BoxCollider2D{{}, {8.0F, 0.5F}, 0.0F};
    if (!engine.physics2d().create_body(ground)) return 2;

    physics2d::BodyDescription2D player;
    player.type = physics2d::BodyType::Dynamic;
    player.position = {0.0F, 3.0F};
    player.collider = physics2d::CircleCollider2D{{}, 0.5F};
    const auto player_body = engine.physics2d().create_body(player);
    if (!player_body) return 3;

    ui::UiNodeDescription panel;
    panel.type = ui::UiNodeType::Panel;
    panel.style.width = ui::UiSize::fixed(220.0F);
    panel.style.height = ui::UiSize::fixed(48.0F);
    panel.style.padding = 8.0F;
    panel.style.background = {0.04F, 0.06F, 0.1F, 0.9F};
    const auto panel_handle = engine.ui().create_node({}, panel);
    examples::BasicUiMetrics ui_metrics;
    if (!panel_handle ||
        engine.ui().layout({0, 0, 800, 600}, ui_metrics) != ui::UiError::None)
        return 4;

    world::Camera2D camera;
    world::CameraMatrices2D camera_matrices;
    if (world::build_camera_matrices(camera, 800U, 600U, camera_matrices) !=
        world::CameraBuildError::None)
        return 5;

    for (unsigned frame = 0; frame < 180U; ++frame) {
        const auto update = engine.update(1.0 / 60.0);
        if (!update) return 6;
        if (!engine.begin_render_frame({0.025F, 0.04F, 0.075F, 1.0F})) return 7;
        if (engine.frame_builder().set_camera_2d(camera_matrices) !=
            render::FrameBuilderError::None)
            return 8;

        const auto state = engine.physics2d().body_state(player_body.handle);
        if (!state) return 9;
        world::Transform2D transform;
        transform.position = {state->position.x, state->position.y};
        transform.rotation_radians = state->rotation_radians;
        transform.scale = {0.8F, 0.8F};
        render::SpriteQuad2D sprite;
        sprite.transform = world::transform_matrix(transform);
        sprite.tint = {0.2F, 0.75F, 1.0F, 1.0F};
        sprite.layer = 0;
        if (engine.frame_builder().add_quad(sprite) !=
            render::FrameBuilderError::None)
            return 10;

        if (engine.enqueue_ui_draws(camera, {0, 0, 800, 600}) !=
            ui::UiRenderError::None)
            return 11;
        if (!engine.render_frame()) return 12;
    }

    const auto final_player = engine.physics2d().body_state(player_body.handle);
    const bool settled = final_player && final_player->position.y > -1.2F &&
                         final_player->position.y < -0.8F;
    const bool drew_scene_and_ui = host.frames == 180U &&
        host.last_2d_draws == 2U && host.last_3d_draws == 0U;
    engine.shutdown();
    if (!settled || !drew_scene_and_ui) {
        std::cerr << "2D sample failed physics or render integration checks\n";
        return 13;
    }
    std::cout << "2D sample passed: 180 fixed-step frames, player settled, UI rendered.\n";
    return 0;
}
