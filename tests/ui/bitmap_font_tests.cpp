#include <dream/ui/bitmap_font.hpp>

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string_view>
#include <vector>

namespace {
void require(bool condition, std::string_view message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}
}

int main() {
    dream::ui::BitmapGlyphProvider5x7 font;
    std::vector<dream::ui::UiGlyphQuad> glyphs;
    require(font.build_glyphs(
        "Tap to jump!", 14.0F, {10.0F, 20.0F, 160.0F, 18.0F}, glyphs),
        "ASCII text builds into bitmap glyph runs");
    require(!glyphs.empty() && glyphs.size() < 512U,
        "glyph runs stay compact for a short label");
    for (const auto& glyph : glyphs) {
        require(glyph.bounds.width > 0.0F && glyph.bounds.height > 0.0F,
            "glyph quads have positive dimensions");
        require(glyph.texture_id == 0U && glyph.uv.valid(),
            "glyph runs use the renderer's default white texture and full UVs");
        require(glyph.bounds.x >= 10.0F && glyph.bounds.y >= 20.0F,
            "glyph positions remain relative to the supplied text bounds");
    }

    const auto previous_size = glyphs.size();
    require(!font.build_glyphs(
        "invalid", std::numeric_limits<float>::quiet_NaN(),
        {0.0F, 0.0F, 100.0F, 20.0F}, glyphs),
        "non-finite font sizes are rejected");
    require(glyphs.size() == previous_size,
        "failed glyph generation preserves the caller's previous output");

    std::vector<dream::ui::UiGlyphQuad> fallback;
    require(font.build_glyphs(
        "?", 16.0F, {0.0F, 0.0F, 16.0F, 16.0F}, fallback) &&
        !fallback.empty(),
        "unsupported characters can use the question-mark fallback");
    return EXIT_SUCCESS;
}
