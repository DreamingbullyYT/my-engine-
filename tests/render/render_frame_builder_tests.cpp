#include <dream/render/render_frame_builder.hpp>

#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string_view>

namespace {
using namespace dream::render;
using namespace dream::world;

void require(bool condition, std::string_view message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

Matrix3x3 translation(float x, float y) {
    Matrix3x3 result;
    result.at(0, 2) = x;
    result.at(1, 2) = y;
    return result;
}
}

int main() {
    RenderFrameBuilder builder(FrameBuilderLimits{4U, 3U});
    require(builder.begin_frame(7U, 1.0 / 60.0, {1280U, 720U},
                {0.1F, 0.2F, 0.3F, 1.0F}) == FrameBuilderError::None,
            "frame begins with valid size and delta");
    require(builder.set_camera_2d(CameraMatrices2D{}) == FrameBuilderError::None,
            "2D camera attaches");

    SpriteQuad2D high_layer;
    high_layer.transform = translation(10.0F, 20.0F);
    high_layer.uv = {0.25F, 0.5F, 0.75F, 1.0F};
    high_layer.tint = {1, 0.5F, 0.25F, 1};
    high_layer.texture_id = 17U;
    high_layer.layer = 5;
    require(builder.add_quad(high_layer) == FrameBuilderError::None,
            "first quad is queued");

    SpriteQuad2D low_layer;
    low_layer.transform = translation(-2.0F, 3.0F);
    low_layer.texture_id = 21U;
    low_layer.layer = -2;
    low_layer.blend = BlendMode::Opaque;
    require(builder.add_quad(low_layer) == FrameBuilderError::None,
            "second quad is queued");

    SpriteQuad2D same_layer;
    same_layer.transform = translation(30.0F, 40.0F);
    same_layer.texture_id = 22U;
    same_layer.layer = 5;
    same_layer.sort_key = 99U;
    require(builder.add_quad(same_layer) == FrameBuilderError::None,
            "third quad is queued");

    require(builder.add_mesh(DrawCommand3D{44U, 55U, Matrix4x4{}, {1, 1, 1, 1}, 20U}) ==
                FrameBuilderError::None,
            "3D mesh command is queued");
    require(builder.add_mesh(DrawCommand3D{66U, 77U, Matrix4x4{}, {1, 1, 1, 1}, 10U}) ==
                FrameBuilderError::None,
            "second 3D mesh command is queued");

    require(builder.finalize() == FrameBuilderError::None && builder.finalized(),
            "frame finalization succeeds");

    const auto frame = builder.packet();
    require(frame.frame_index == 7U && frame.framebuffer_extent == Extent2D{1280U, 720U},
            "packet retains frame metadata");
    require(frame.vertices_2d.size() == 12U &&
            frame.indices_2d.size() == 18U &&
            frame.draws_2d.size() == 3U,
            "three sprites expand into quad vertices, indices and commands");
    require(frame.draws_2d[0].layer == -2 &&
            frame.draws_2d[0].texture_id == 21U,
            "2D commands sort by ascending layer");
    require(frame.draws_2d[1].texture_id == 17U &&
            frame.draws_2d[2].texture_id == 22U,
            "same-layer 2D commands retain submission order");
    require(frame.draws_2d[0].first_index == 0U &&
            frame.draws_2d[0].base_vertex == 0U &&
            frame.draws_2d[1].first_index == 6U &&
            frame.draws_2d[1].base_vertex == 4U &&
            frame.draws_2d[2].first_index == 12U &&
            frame.draws_2d[2].base_vertex == 8U,
            "each sprite has independent indexed quad range");
    require(frame.vertices_2d[4].uv.x == 0.25F &&
            frame.vertices_2d[4].uv.y == 0.5F &&
            frame.vertices_2d[6].uv.x == 0.75F &&
            frame.vertices_2d[6].uv.y == 1.0F,
            "UV rectangle maps to correct quad corners");

    require(frame.draws_3d.size() == 2U &&
            frame.draws_3d[0].mesh_id == 66U &&
            frame.draws_3d[1].mesh_id == 44U,
            "3D draws sort by stable sort key");

    RenderCapabilities capabilities{true, true, true, 16U, 4096U};
    require(validate_frame_packet(frame, capabilities) == FrameValidationError::None,
            "built packet passes renderer validation");

    require(builder.begin_frame(8U, 0.0, {640U, 480U}) == FrameBuilderError::None,
            "begin frame resets prior commands");
    require(builder.quad_count() == 0U && builder.mesh_draw_count() == 0U,
            "frame reset clears queued draw commands");
    require(builder.finalize() == FrameBuilderError::None,
            "empty frame finalizes");
    const auto empty_frame = builder.packet();
    require(empty_frame.frame_index == 8U &&
            empty_frame.draws_2d.empty() &&
            empty_frame.draws_3d.empty(),
            "empty packet contains no stale draw commands");

    RenderFrameBuilder limited(FrameBuilderLimits{1U, 1U});
    require(limited.begin_frame(1U, 0.0, {32U, 32U}) == FrameBuilderError::None,
            "limited builder begins");
    require(limited.add_quad(SpriteQuad2D{}) == FrameBuilderError::None,
            "one quad fits configured capacity");
    require(limited.add_quad(SpriteQuad2D{}) == FrameBuilderError::DrawLimitExceeded,
            "quad capacity is enforced");
    require(limited.finalize() == FrameBuilderError::DrawLimitExceeded,
            "builder preserves first error and refuses partial finalization");

    RenderFrameBuilder invalid;
    require(invalid.begin_frame(1U, 0.0, {10U, 10U}) == FrameBuilderError::None,
            "invalid builder begins");
    SpriteQuad2D bad_uv;
    bad_uv.uv = {0.8F, 0.0F, 0.2F, 1.0F};
    require(invalid.add_quad(bad_uv) == FrameBuilderError::InvalidUvRect,
            "inverted UV rectangle is rejected");
    require(invalid.finalize() == FrameBuilderError::InvalidUvRect,
            "first builder error is sticky");

    RenderFrameBuilder invalid_mesh;
    require(invalid_mesh.begin_frame(1U, 0.0, {10U, 10U}) == FrameBuilderError::None,
            "mesh builder begins");
    require(invalid_mesh.add_mesh(DrawCommand3D{}) == FrameBuilderError::InvalidMeshId,
            "zero mesh id is rejected");
    require(invalid_mesh.add_mesh(DrawCommand3D{1U, 0U}) ==
                FrameBuilderError::InvalidMeshId,
            "builder retains first invalid mesh error");

    std::cout << "Dream Engine render frame builder tests passed.\n";
    return EXIT_SUCCESS;
}
