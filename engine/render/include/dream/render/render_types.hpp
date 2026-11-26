#pragma once

#include <dream/world/camera.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <span>

namespace dream::render {

using RenderAssetId = std::uint64_t;

struct Extent2D final {
    std::uint32_t width{0};
    std::uint32_t height{0};

    [[nodiscard]] constexpr bool valid() const noexcept {
        return width != 0U && height != 0U;
    }

    friend constexpr bool operator==(Extent2D, Extent2D) noexcept = default;
};

struct RenderSurfaceInfo final {
    Extent2D pixel_extent{};
    Extent2D logical_extent{};
    float scale_x{1.0F};
    float scale_y{1.0F};
    bool focused{false};
};

struct Color final {
    float r{0.0F};
    float g{0.0F};
    float b{0.0F};
    float a{1.0F};
};

struct RenderCapabilities final {
    bool supports_2d{false};
    bool supports_3d{false};
    bool supports_instancing{false};
    std::uint32_t maximum_draws_per_frame{
        std::numeric_limits<std::uint32_t>::max()};
    std::uint32_t maximum_texture_dimension{0};
};

enum class BlendMode : std::uint8_t {
    Opaque,
    Alpha,
    Additive
};

struct Vertex2D final {
    world::Vec2 position{};
    world::Vec2 uv{};
    Color color{};
};

struct Vertex3D final {
    world::Vec3 position{};
    world::Vec3 normal{0.0F,1.0F,0.0F};
    world::Vec2 uv{};
};

struct MeshDescription final {
    std::span<const Vertex3D> vertices{};
    std::span<const std::uint32_t> indices{};
};

struct MaterialDescription final {
    Color base_color{1.0F,1.0F,1.0F,1.0F};
    RenderAssetId texture_id{0};
    BlendMode blend{BlendMode::Opaque};
    bool depth_test{true};
    bool depth_write{true};
    bool backface_culling{true};
};

// Texture ID zero denotes the renderer's default white texture.
struct DrawCommand2D final {
    std::uint32_t first_index{0};
    std::uint32_t index_count{0};
    std::uint32_t base_vertex{0};
    RenderAssetId texture_id{0};
    world::Matrix3x3 transform{};
    Color tint{};
    std::int32_t layer{0};
    std::uint64_t sort_key{0};
    BlendMode blend{BlendMode::Alpha};
};

// Mesh and material IDs are stable engine asset identifiers, not GPU handles.
struct DrawCommand3D final {
    RenderAssetId mesh_id{0};
    RenderAssetId material_id{0};
    world::Matrix4x4 model{};
    Color tint{};
    std::uint64_t sort_key{0};
};

struct RenderFramePacket final {
    std::uint64_t frame_index{0};
    double delta_seconds{0.0};
    Extent2D framebuffer_extent{};
    Color clear_color{};
    bool clear_color_buffer{true};
    bool clear_depth_buffer{true};
    std::optional<world::CameraMatrices2D> camera_2d;
    std::optional<world::CameraMatrices3D> camera_3d;
    std::span<const Vertex2D> vertices_2d{};
    std::span<const std::uint32_t> indices_2d{};
    std::span<const DrawCommand2D> draws_2d{};
    std::span<const DrawCommand3D> draws_3d{};
};

enum class FrameValidationError : std::uint8_t {
    None,
    InvalidExtent,
    InvalidDelta,
    InvalidClearColor,
    Unsupported2D,
    Unsupported3D,
    MissingCamera,
    InvalidCamera,
    TooManyDraws,
    InvalidVertex,
    InvalidIndexRange,
    InvalidTextureId,
    InvalidMeshId,
    InvalidMaterialId,
    InvalidTransform,
    InvalidTint,
    InvalidBlendMode
};

[[nodiscard]] inline bool finite(Color color) noexcept {
    return std::isfinite(color.r) && std::isfinite(color.g) &&
           std::isfinite(color.b) && std::isfinite(color.a);
}

[[nodiscard]] inline bool finite(const world::Matrix3x3& matrix) noexcept {
    return std::all_of(matrix.values.begin(), matrix.values.end(),
        [](float value) { return std::isfinite(value); });
}

[[nodiscard]] inline bool finite(const world::Matrix4x4& matrix) noexcept {
    return std::all_of(matrix.values.begin(), matrix.values.end(),
        [](float value) { return std::isfinite(value); });
}

[[nodiscard]] inline FrameValidationError validate_frame_packet(
    const RenderFramePacket& frame, const RenderCapabilities& capabilities) noexcept {
    if (!frame.framebuffer_extent.valid())
        return FrameValidationError::InvalidExtent;
    if (!std::isfinite(frame.delta_seconds) || frame.delta_seconds < 0.0)
        return FrameValidationError::InvalidDelta;
    if (!finite(frame.clear_color))
        return FrameValidationError::InvalidClearColor;

    const std::uint64_t draw_count =
        static_cast<std::uint64_t>(frame.draws_2d.size()) +
        static_cast<std::uint64_t>(frame.draws_3d.size());
    if (draw_count > capabilities.maximum_draws_per_frame)
        return FrameValidationError::TooManyDraws;

    if (!frame.draws_2d.empty()) {
        if (!capabilities.supports_2d)
            return FrameValidationError::Unsupported2D;
        if (!frame.camera_2d)
            return FrameValidationError::MissingCamera;
        if (!finite(frame.camera_2d->view) ||
            !finite(frame.camera_2d->projection) ||
            !finite(frame.camera_2d->view_projection)) {
            return FrameValidationError::InvalidCamera;
        }
    }

    if (!frame.draws_3d.empty()) {
        if (!capabilities.supports_3d)
            return FrameValidationError::Unsupported3D;
        if (!frame.camera_3d)
            return FrameValidationError::MissingCamera;
        if (!finite(frame.camera_3d->view) ||
            !finite(frame.camera_3d->projection) ||
            !finite(frame.camera_3d->view_projection)) {
            return FrameValidationError::InvalidCamera;
        }
    }

    for (const auto& vertex : frame.vertices_2d) {
        if (!std::isfinite(vertex.position.x) ||
            !std::isfinite(vertex.position.y) ||
            !std::isfinite(vertex.uv.x) || !std::isfinite(vertex.uv.y) ||
            !finite(vertex.color)) {
            return FrameValidationError::InvalidVertex;
        }
    }

    for (const auto& draw : frame.draws_2d) {
        if (draw.index_count == 0U || draw.index_count % 3U != 0U ||
            draw.first_index > frame.indices_2d.size() ||
            draw.index_count > frame.indices_2d.size() - draw.first_index) {
            return FrameValidationError::InvalidIndexRange;
        }
        if (!finite(draw.transform))
            return FrameValidationError::InvalidTransform;
        if (!finite(draw.tint))
            return FrameValidationError::InvalidTint;
        switch (draw.blend) {
        case BlendMode::Opaque:
        case BlendMode::Alpha:
        case BlendMode::Additive:
            break;
        default:
            return FrameValidationError::InvalidBlendMode;
        }

        for (std::size_t offset = 0; offset < draw.index_count; ++offset) {
            const std::uint64_t vertex_index =
                static_cast<std::uint64_t>(
                    frame.indices_2d[draw.first_index + offset]) +
                draw.base_vertex;
            if (vertex_index >= frame.vertices_2d.size())
                return FrameValidationError::InvalidIndexRange;
        }
    }

    for (const auto& draw : frame.draws_3d) {
        if (draw.mesh_id == 0U)
            return FrameValidationError::InvalidMeshId;
        if (draw.material_id == 0U)
            return FrameValidationError::InvalidMaterialId;
        if (!finite(draw.model))
            return FrameValidationError::InvalidTransform;
        if (!finite(draw.tint))
            return FrameValidationError::InvalidTint;
    }

[[nodiscard]] inline FrameValidationError validate_mesh_description(
    const MeshDescription& mesh) noexcept {
    if (mesh.vertices.empty() || mesh.indices.empty() ||
        mesh.indices.size() % 3U != 0U ||
        mesh.vertices.size() > std::numeric_limits<std::uint32_t>::max() ||
        mesh.indices.size() > std::numeric_limits<std::uint32_t>::max())
        return FrameValidationError::InvalidIndexRange;
    for (const auto& vertex : mesh.vertices) {
        if (!std::isfinite(vertex.position.x) ||
            !std::isfinite(vertex.position.y) ||
            !std::isfinite(vertex.position.z) ||
            !std::isfinite(vertex.normal.x) ||
            !std::isfinite(vertex.normal.y) ||
            !std::isfinite(vertex.normal.z) ||
            !std::isfinite(vertex.uv.x) ||
            !std::isfinite(vertex.uv.y))
            return FrameValidationError::InvalidVertex;
        const float normal_length_squared =
            vertex.normal.x * vertex.normal.x +
            vertex.normal.y * vertex.normal.y +
            vertex.normal.z * vertex.normal.z;
        if (!std::isfinite(normal_length_squared) ||
            normal_length_squared <= 1.0e-12F)
            return FrameValidationError::InvalidVertex;
    }
    for (const auto index : mesh.indices)
        if (index >= mesh.vertices.size())
            return FrameValidationError::InvalidIndexRange;
    return FrameValidationError::None;
}

[[nodiscard]] inline bool valid_material_description(
    const MaterialDescription& material) noexcept {
    if (!finite(material.base_color)) return false;
    for (const float channel : {
        material.base_color.r, material.base_color.g,
        material.base_color.b, material.base_color.a}) {
        if (channel < 0.0F || channel > 1.0F) return false;
    }
    switch (material.blend) {
    case BlendMode::Opaque:
    case BlendMode::Alpha:
    case BlendMode::Additive:
        return true;
    }
    return false;
}


    return FrameValidationError::None;
}

} // namespace dream::render
