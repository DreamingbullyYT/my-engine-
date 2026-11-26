#pragma once

#include <dream/render/render_types.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <span>
#include <utility>
#include <vector>

namespace dream::render {

struct UvRect final {
    float left{0.0F};
    float top{0.0F};
    float right{1.0F};
    float bottom{1.0F};

    [[nodiscard]] bool valid() const noexcept {
        return std::isfinite(left) && std::isfinite(top) &&
               std::isfinite(right) && std::isfinite(bottom) &&
               left <= right && top <= bottom &&
               left >= 0.0F && top >= 0.0F &&
               right <= 1.0F && bottom <= 1.0F;
    }
};

struct SpriteQuad2D final {
    world::Matrix3x3 transform{};
    UvRect uv{};
    Color tint{};
    RenderAssetId texture_id{0};
    std::int32_t layer{0};
    std::uint64_t sort_key{0};
    BlendMode blend{BlendMode::Alpha};
};

struct FrameBuilderLimits final {
    std::size_t maximum_quads_2d{100000U};
    std::size_t maximum_mesh_draws_3d{100000U};
};

enum class FrameBuilderError : std::uint8_t {
    None,
    InvalidFrameExtent,
    InvalidDelta,
    InvalidClearColor,
    InvalidCamera,
    InvalidQuadTransform,
    InvalidUvRect,
    InvalidTint,
    InvalidBlend,
    InvalidMeshId,
    InvalidMaterialId,
    DrawLimitExceeded,
    IndexCapacityExceeded
};

class RenderFrameBuilder final {
public:
    explicit RenderFrameBuilder(FrameBuilderLimits limits = {})
        : limits_(limits) {}

    RenderFrameBuilder(const RenderFrameBuilder&) = delete;
    RenderFrameBuilder& operator=(const RenderFrameBuilder&) = delete;

    [[nodiscard]] FrameBuilderError begin_frame(
        std::uint64_t frame_index, double delta_seconds,
        Extent2D framebuffer_extent, Color clear_color = {},
        bool clear_color_buffer = true, bool clear_depth_buffer = true) {
        reset();
        if (!framebuffer_extent.valid())
            return set_error(FrameBuilderError::InvalidFrameExtent);
        if (!std::isfinite(delta_seconds) || delta_seconds < 0.0)
            return set_error(FrameBuilderError::InvalidDelta);
        if (!finite(clear_color))
            return set_error(FrameBuilderError::InvalidClearColor);

        frame_index_ = frame_index;
        delta_seconds_ = delta_seconds;
        framebuffer_extent_ = framebuffer_extent;
        clear_color_ = clear_color;
        clear_color_buffer_ = clear_color_buffer;
        clear_depth_buffer_ = clear_depth_buffer;
        begun_ = true;
        return FrameBuilderError::None;
    }

    [[nodiscard]] FrameBuilderError set_camera_2d(
        const world::CameraMatrices2D& camera) {
        if (!begun_) return set_error(FrameBuilderError::InvalidFrameExtent);
        if (!finite(camera.view) || !finite(camera.projection) ||
            !finite(camera.view_projection)) {
            return set_error(FrameBuilderError::InvalidCamera);
        }
        camera_2d_ = camera;
        return FrameBuilderError::None;
    }

    [[nodiscard]] FrameBuilderError set_camera_3d(
        const world::CameraMatrices3D& camera) {
        if (!begun_) return set_error(FrameBuilderError::InvalidFrameExtent);
        if (!finite(camera.view) || !finite(camera.projection) ||
            !finite(camera.view_projection)) {
            return set_error(FrameBuilderError::InvalidCamera);
        }
        camera_3d_ = camera;
        return FrameBuilderError::None;
    }

    [[nodiscard]] FrameBuilderError add_quad(SpriteQuad2D quad) {
        if (!begun_) return set_error(FrameBuilderError::InvalidFrameExtent);
        if (!finite(quad.transform))
            return set_error(FrameBuilderError::InvalidQuadTransform);
        if (!quad.uv.valid())
            return set_error(FrameBuilderError::InvalidUvRect);
        if (!finite(quad.tint))
            return set_error(FrameBuilderError::InvalidTint);
        if (!valid_blend(quad.blend))
            return set_error(FrameBuilderError::InvalidBlend);
        if (quads_.size() >= limits_.maximum_quads_2d)
            return set_error(FrameBuilderError::DrawLimitExceeded);
        quads_.push_back({std::move(quad), next_sequence_++});
        finalized_ = false;
        return FrameBuilderError::None;
    }

    [[nodiscard]] FrameBuilderError add_mesh(DrawCommand3D draw) {
        if (!begun_) return set_error(FrameBuilderError::InvalidFrameExtent);
        if (draw.mesh_id == 0U)
            return set_error(FrameBuilderError::InvalidMeshId);
        if (draw.material_id == 0U)
            return set_error(FrameBuilderError::InvalidMaterialId);
        if (!finite(draw.model))
            return set_error(FrameBuilderError::InvalidQuadTransform);
        if (!finite(draw.tint))
            return set_error(FrameBuilderError::InvalidTint);
        if (mesh_draws_.size() >= limits_.maximum_mesh_draws_3d)
            return set_error(FrameBuilderError::DrawLimitExceeded);
        mesh_draws_.push_back({std::move(draw), next_sequence_++});
        finalized_ = false;
        return FrameBuilderError::None;
    }

    [[nodiscard]] FrameBuilderError finalize() {
        if (!begun_) return set_error(FrameBuilderError::InvalidFrameExtent);
        if (error_ != FrameBuilderError::None) return error_;

        const std::size_t vertex_count = quads_.size() * 4U;
        const std::size_t index_count = quads_.size() * 6U;
        if (vertex_count > std::numeric_limits<std::uint32_t>::max() ||
            index_count > std::numeric_limits<std::uint32_t>::max()) {
            return set_error(FrameBuilderError::IndexCapacityExceeded);
        }

        std::stable_sort(quads_.begin(), quads_.end(),
            [](const QueuedQuad& left, const QueuedQuad& right) {
                if (left.quad.layer != right.quad.layer)
                    return left.quad.layer < right.quad.layer;
                return left.sequence < right.sequence;
            });
        std::stable_sort(mesh_draws_.begin(), mesh_draws_.end(),
            [](const QueuedMesh& left, const QueuedMesh& right) {
                if (left.draw.sort_key != right.draw.sort_key)
                    return left.draw.sort_key < right.draw.sort_key;
                return left.sequence < right.sequence;
            });

        vertices_.clear();
        indices_.clear();
        draws_2d_.clear();
        draws_3d_.clear();
        vertices_.reserve(vertex_count);
        indices_.reserve(index_count);
        draws_2d_.reserve(quads_.size());
        draws_3d_.reserve(mesh_draws_.size());

        constexpr std::array<world::Vec2, 4> corners{{
            {0.0F, 0.0F}, {1.0F, 0.0F},
            {1.0F, 1.0F}, {0.0F, 1.0F}
        }};
        constexpr std::array<std::uint32_t, 6> quad_indices{
            0U, 1U, 2U, 2U, 3U, 0U
        };

        for (const auto& queued : quads_) {
            const auto& quad = queued.quad;
            const auto base_vertex = static_cast<std::uint32_t>(vertices_.size());
            const auto first_index = static_cast<std::uint32_t>(indices_.size());

            const std::array<world::Vec2, 4> uvs{{
                {quad.uv.left, quad.uv.top},
                {quad.uv.right, quad.uv.top},
                {quad.uv.right, quad.uv.bottom},
                {quad.uv.left, quad.uv.bottom}
            }};

            for (std::size_t index = 0; index < corners.size(); ++index) {
                vertices_.push_back({
                    corners[index], uvs[index], Color{1.0F, 1.0F, 1.0F, 1.0F}
                });
            }
            for (const auto index : quad_indices) indices_.push_back(index);

            draws_2d_.push_back({
                first_index,
                6U,
                base_vertex,
                quad.texture_id,
                quad.transform,
                quad.tint,
                quad.layer,
                quad.sort_key,
                quad.blend
            });
        }

        for (const auto& queued : mesh_draws_)
            draws_3d_.push_back(queued.draw);

        finalized_ = true;
        return FrameBuilderError::None;
    }

    [[nodiscard]] bool finalized() const noexcept { return finalized_; }
    [[nodiscard]] FrameBuilderError error() const noexcept { return error_; }
    [[nodiscard]] std::size_t quad_count() const noexcept { return quads_.size(); }
    [[nodiscard]] std::size_t mesh_draw_count() const noexcept {
        return mesh_draws_.size();
    }

    [[nodiscard]] RenderFramePacket packet() const noexcept {
        RenderFramePacket frame;
        frame.frame_index = frame_index_;
        frame.delta_seconds = delta_seconds_;
        frame.framebuffer_extent = framebuffer_extent_;
        frame.clear_color = clear_color_;
        frame.clear_color_buffer = clear_color_buffer_;
        frame.clear_depth_buffer = clear_depth_buffer_;
        frame.camera_2d = camera_2d_;
        frame.camera_3d = camera_3d_;
        frame.vertices_2d = vertices_;
        frame.indices_2d = indices_;
        frame.draws_2d = draws_2d_;
        frame.draws_3d = draws_3d_;
        return frame;
    }

    void reset() noexcept {
        frame_index_ = 0;
        delta_seconds_ = 0.0;
        framebuffer_extent_ = {};
        clear_color_ = {};
        clear_color_buffer_ = true;
        clear_depth_buffer_ = true;
        camera_2d_.reset();
        camera_3d_.reset();
        quads_.clear();
        mesh_draws_.clear();
        vertices_.clear();
        indices_.clear();
        draws_2d_.clear();
        draws_3d_.clear();
        next_sequence_ = 0;
        error_ = FrameBuilderError::None;
        begun_ = false;
        finalized_ = false;
    }

private:
    struct QueuedQuad final {
        SpriteQuad2D quad;
        std::uint64_t sequence{0};
    };

    struct QueuedMesh final {
        DrawCommand3D draw;
        std::uint64_t sequence{0};
    };

    [[nodiscard]] FrameBuilderError set_error(FrameBuilderError error) noexcept {
        if (error_ == FrameBuilderError::None) error_ = error;
        return error_;
    }

    [[nodiscard]] static bool valid_blend(BlendMode blend) noexcept {
        switch (blend) {
        case BlendMode::Opaque:
        case BlendMode::Alpha:
        case BlendMode::Additive:
            return true;
        }
        return false;
    }

    FrameBuilderLimits limits_{};
    std::uint64_t frame_index_{0};
    double delta_seconds_{0.0};
    Extent2D framebuffer_extent_{};
    Color clear_color_{};
    bool clear_color_buffer_{true};
    bool clear_depth_buffer_{true};
    std::optional<world::CameraMatrices2D> camera_2d_;
    std::optional<world::CameraMatrices3D> camera_3d_;
    std::vector<QueuedQuad> quads_;
    std::vector<QueuedMesh> mesh_draws_;
    std::vector<Vertex2D> vertices_;
    std::vector<std::uint32_t> indices_;
    std::vector<DrawCommand2D> draws_2d_;
    std::vector<DrawCommand3D> draws_3d_;
    std::uint64_t next_sequence_{0};
    FrameBuilderError error_{FrameBuilderError::None};
    bool begun_{false};
    bool finalized_{false};
};

} // namespace dream::render
