#pragma once

#include <dream/render/render_frame_builder.hpp>
#include <dream/ui/ui_runtime.hpp>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>
#include <utility>
#include <vector>

namespace dream::ui {

struct UiGlyphQuad final {
    UiRect bounds{};
    render::UvRect uv{};
    render::RenderAssetId texture_id{0};
};

class IUiGlyphProvider {
public:
    virtual ~IUiGlyphProvider() = default;

    // Glyph bounds are expressed in the same logical coordinates as UiRuntime.
    // The provider owns font shaping, fallback, atlas packing and glyph metrics.
    [[nodiscard]] virtual bool build_glyphs(
        std::string_view text, float font_size, UiRect text_bounds,
        std::vector<UiGlyphQuad>& output) const = 0;
};

enum class UiRenderError : std::uint8_t {
    None,
    InvalidViewport,
    InvalidCamera,
    InvalidGeometry,
    MissingGlyphProvider,
    GlyphLayoutFailed,
    GlyphLimitExceeded,
    FrameNotOpen,
    RuntimeNotInitialized,
    FrameBuilderRejected
};

// Bridges retained UI drawing into the engine's normal indexed 2D sprite queue.
// The supplied camera must be the same Camera2D used to build the frame packet;
// the bridge maps logical top-left UI coordinates back into that camera's world
// space so UI positions remain screen-aligned without adding a second render pass.
class UiRenderAdapter final : public IUiDrawSink {
public:
    static constexpr std::size_t maximum_glyph_quads = 16384U;

    UiRenderAdapter(
        render::RenderFrameBuilder& builder, UiRect viewport,
        world::Camera2D camera, const IUiGlyphProvider* glyph_provider = nullptr,
        std::int32_t layer = 100000) noexcept
        : builder_(builder), viewport_(viewport), camera_(camera),
          glyph_provider_(glyph_provider), layer_(layer) {
        valid_ = finite_rect(viewport_) && !viewport_.empty() &&
            std::isfinite(camera_.position.x) &&
            std::isfinite(camera_.position.y) &&
            std::isfinite(camera_.rotation_radians) &&
            std::isfinite(camera_.orthographic_height) &&
            std::isfinite(camera_.zoom) &&
            camera_.orthographic_height > 0.0F && camera_.zoom > 0.0F;
        if (valid_ && !builder_.accepting_draws()) {
            valid_ = false;
            error_ = UiRenderError::FrameNotOpen;
        }
        if (valid_) {
            world_height_ = camera_.orthographic_height / camera_.zoom;
            world_width_ = world_height_ * viewport_.width / viewport_.height;
            valid_ = std::isfinite(world_width_) &&
                std::isfinite(world_height_) &&
                world_width_ > 0.0F && world_height_ > 0.0F;
        }
        cosine_ = std::cos(camera_.rotation_radians);
        sine_ = std::sin(camera_.rotation_radians);
        if (!valid_ && error_ == UiRenderError::None)
            error_ = UiRenderError::InvalidViewport;
    }

    [[nodiscard]] UiRenderError error() const noexcept { return error_; }
    [[nodiscard]] bool valid() const noexcept { return valid_; }
    [[nodiscard]] std::size_t submitted_quad_count() const noexcept {
        return submitted_quad_count_;
    }

    void draw_rect(
        UiRect rect, UiRect clip, UiColor color, float /*corner_radius*/) override {
        if (!can_draw()) return;
        const auto clipped = clip_quad(rect, clip, {0, 0, 1, 1});
        if (!clipped) return;
        (void)submit(*clipped, 0U, to_render_color(color));
        // The current sprite pipeline renders rectangular quads. Corner-radius
        // masking requires a dedicated UI shader and is intentionally not faked.
    }

    void draw_text(
        UiRect rect, UiRect clip, std::string_view text, float font_size,
        UiColor color) override {
        if (!can_draw() || text.empty()) return;
        if (glyph_provider_ == nullptr) {
            set_error(UiRenderError::MissingGlyphProvider);
            return;
        }

        std::vector<UiGlyphQuad> glyphs;
        if (!glyph_provider_->build_glyphs(text, font_size, rect, glyphs)) {
            set_error(UiRenderError::GlyphLayoutFailed);
            return;
        }
        if (glyphs.size() > maximum_glyph_quads ||
            glyphs.size() > maximum_glyph_quads - submitted_quad_count_) {
            set_error(UiRenderError::GlyphLimitExceeded);
            return;
        }

        const auto tint = to_render_color(color);
        for (const auto& glyph : glyphs) {
            const auto clipped = clip_quad(glyph.bounds, clip, glyph.uv);
            if (!clipped) continue;
            if (!submit(*clipped, glyph.texture_id, tint)) return;
        }
    }

    void draw_image(
        UiRect rect, UiRect clip, std::uint64_t image_id, UiColor tint) override {
        if (!can_draw() || image_id == 0U) return;
        const auto clipped = clip_quad(rect, clip, {0, 0, 1, 1});
        if (!clipped) return;
        (void)submit(*clipped, image_id, to_render_color(tint));
    }

private:
    struct ClippedQuad final {
        UiRect rect{};
        render::UvRect uv{};
    };

    [[nodiscard]] static bool finite_rect(UiRect rect) noexcept {
        return std::isfinite(rect.x) && std::isfinite(rect.y) &&
            std::isfinite(rect.width) && std::isfinite(rect.height) &&
            rect.width >= 0.0F && rect.height >= 0.0F;
    }

    [[nodiscard]] static render::Color to_render_color(UiColor color) noexcept {
        return {color.r, color.g, color.b, color.a};
    }

    void set_error(UiRenderError error) noexcept {
        if (error_ == UiRenderError::None) error_ = error;
    }

    [[nodiscard]] bool can_draw() noexcept {
        if (!valid_) return false;
        if (!builder_.accepting_draws()) {
            set_error(UiRenderError::FrameNotOpen);
            return false;
        }
        return error_ == UiRenderError::None;
    }

    [[nodiscard]] static std::optional<ClippedQuad> clip_quad(
        UiRect bounds, UiRect clip, render::UvRect uv) noexcept {
        if (!finite_rect(bounds) || !finite_rect(clip) || !uv.valid()) {
            return std::nullopt;
        }
        if (bounds.empty() || clip.empty()) return std::nullopt;
        const UiRect clipped = intersect(bounds, clip);
        if (clipped.empty()) return std::nullopt;

        const float left = (clipped.x - bounds.x) / bounds.width;
        const float top = (clipped.y - bounds.y) / bounds.height;
        const float right = (clipped.x + clipped.width - bounds.x) / bounds.width;
        const float bottom = (clipped.y + clipped.height - bounds.y) / bounds.height;
        return ClippedQuad{
            clipped,
            {
                uv.left + (uv.right - uv.left) * left,
                uv.top + (uv.bottom - uv.top) * top,
                uv.left + (uv.right - uv.left) * right,
                uv.top + (uv.bottom - uv.top) * bottom
            }
        };
    }

    [[nodiscard]] bool submit(
        const ClippedQuad& quad, render::RenderAssetId texture,
        render::Color tint) {
        const float scale_x = world_width_ / viewport_.width * quad.rect.width;
        const float scale_y = world_height_ / viewport_.height * quad.rect.height;
        const float local_x = -world_width_ * 0.5F +
            (quad.rect.x - viewport_.x) * world_width_ / viewport_.width;
        const float local_y = world_height_ * 0.5F -
            (quad.rect.y - viewport_.y) * world_height_ / viewport_.height;

        world::Matrix3x3 transform;
        transform.at(0, 0) = cosine_ * scale_x;
        transform.at(1, 0) = sine_ * scale_x;
        transform.at(0, 1) = sine_ * scale_y;
        transform.at(1, 1) = -cosine_ * scale_y;
        transform.at(0, 2) = camera_.position.x +
            cosine_ * local_x - sine_ * local_y;
        transform.at(1, 2) = camera_.position.y +
            sine_ * local_x + cosine_ * local_y;

        render::SpriteQuad2D sprite;
        sprite.transform = transform;
        sprite.uv = quad.uv;
        sprite.tint = tint;
        sprite.texture_id = texture;
        sprite.layer = layer_;
        sprite.blend = render::BlendMode::Alpha;
        if (builder_.add_quad(std::move(sprite)) != render::FrameBuilderError::None) {
            set_error(UiRenderError::FrameBuilderRejected);
            return false;
        }
        ++submitted_quad_count_;
        return true;
    }

    render::RenderFrameBuilder& builder_;
    UiRect viewport_{};
    world::Camera2D camera_{};
    const IUiGlyphProvider* glyph_provider_{nullptr};
    std::int32_t layer_{100000};
    float world_width_{0.0F};
    float world_height_{0.0F};
    float cosine_{1.0F};
    float sine_{0.0F};
    std::size_t submitted_quad_count_{0};
    UiRenderError error_{UiRenderError::None};
    bool valid_{false};
};

} // namespace dream::ui
