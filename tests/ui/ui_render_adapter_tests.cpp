#include <dream/ui/ui_render_adapter.hpp>

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string_view>

namespace {
void require(bool ok, std::string_view message) {
    if (!ok) {
        std::cerr << "FAILED: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class Glyphs final : public dream::ui::IUiGlyphProvider {
public:
    bool build_glyphs(
        std::string_view text, float, dream::ui::UiRect bounds,
        std::vector<dream::ui::UiGlyphQuad>& output) const override {
        if (text.empty()) return true;
        output.push_back({
            {bounds.x + 5.0F, bounds.y + 2.0F, 12.0F, 16.0F},
            {0.25F, 0.25F, 0.75F, 0.75F},
            77U
        });
        return true;
    }
};
}

int main() {
    using namespace dream;
    render::RenderFrameBuilder builder;
    require(builder.begin_frame(1U, 1.0 / 60.0, {200U, 100U}) ==
                render::FrameBuilderError::None,
            "render frame begins");
    world::Camera2D camera;
    camera.position = {10.0F, -4.0F};
    camera.rotation_radians = 0.35F;
    camera.orthographic_height = 100.0F;
    camera.zoom = 1.0F;
    world::CameraMatrices2D camera_matrices;
    require(world::build_camera_matrices(camera, 200U, 100U, camera_matrices) ==
                world::CameraBuildError::None &&
            builder.set_camera_2d(camera_matrices) == render::FrameBuilderError::None,
            "frame uses the matching UI world camera");

    Glyphs glyphs;
    ui::UiRenderAdapter adapter(
        builder, {0.0F, 0.0F, 200.0F, 100.0F}, camera, &glyphs);
    require(adapter.valid(), "adapter accepts a valid viewport and camera");

    adapter.draw_rect({10, 10, 40, 20}, {20, 10, 30, 20},
                      {0.1F, 0.2F, 0.3F, 1.0F}, 4.0F);
    adapter.draw_image({60, 10, 40, 30}, {70, 15, 20, 20}, 42U,
                       {1, 1, 1, 1});
    adapter.draw_text({100, 10, 50, 25}, {100, 10, 30, 25},
                      "Hi", 16.0F, {1, 1, 1, 1});
    require(adapter.error() == ui::UiRenderError::None &&
            adapter.submitted_quad_count() == 3U,
            "rectangles, clipped images and glyphs are queued as sprite quads");

    require(builder.finalize() == render::FrameBuilderError::None,
            "UI-generated sprite commands finalize");
    const auto frame = builder.packet();
    require(frame.draws_2d.size() == 3U && frame.vertices_2d.size() == 12U,
            "UI adapter contributes indexed sprite geometry");
    require(frame.draws_2d[0].texture_id == 0U &&
            frame.draws_2d[1].texture_id == 42U &&
            frame.draws_2d[2].texture_id == 77U,
            "UI rectangles, images and glyphs select their expected textures");
    require(std::abs(frame.vertices_2d[4].uv.x - 0.25F) < 1.0e-5F &&
            std::abs(frame.vertices_2d[4].uv.y - 1.0F / 6.0F) < 1.0e-5F &&
            std::abs(frame.vertices_2d[6].uv.x - 0.75F) < 1.0e-5F &&
            std::abs(frame.vertices_2d[6].uv.y - 5.0F / 6.0F) < 1.0e-5F,
            "image clipping proportionally crops texture coordinates");
    require(render::validate_frame_packet(
                frame, render::RenderCapabilities{true, false, false, 16U, 4096U}) ==
                render::FrameValidationError::None,
            "UI sprite packet satisfies the backend render contract");

    render::RenderFrameBuilder no_font_builder;
    require(no_font_builder.begin_frame(2U, 0.0, {100U, 100U}) ==
                render::FrameBuilderError::None,
            "missing-font test frame begins");
    ui::UiRenderAdapter no_font(
        no_font_builder, {0, 0, 100, 100}, world::Camera2D{});
    no_font.draw_text({0, 0, 50, 20}, {0, 0, 50, 20},
                      "No font", 14.0F, {1, 1, 1, 1});
    require(no_font.error() == ui::UiRenderError::MissingGlyphProvider,
            "text without a glyph provider reports an explicit error");

    ui::UiRenderAdapter invalid(
        builder, {0, 0, 0, 10}, world::Camera2D{});
    require(!invalid.valid() &&
            invalid.error() == ui::UiRenderError::InvalidViewport,
            "empty UI viewport is rejected");
    return EXIT_SUCCESS;
}
