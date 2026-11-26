#pragma once

#include <dream/ui/ui_render_adapter.hpp>

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

namespace dream::ui {

// Small dependency-free ASCII font for bootstrap/runtime UI. Applications may
// replace it with a shaped font atlas provider without changing UiRenderAdapter.
class BitmapGlyphProvider5x7 final : public IUiGlyphProvider {
public:
    static constexpr std::size_t maximum_quads = 16384U;

    [[nodiscard]] bool build_glyphs(
        std::string_view text, float font_size, UiRect text_bounds,
        std::vector<UiGlyphQuad>& output) const override {
        if (!std::isfinite(font_size) || font_size <= 0.0F ||
            font_size > 512.0F || !finite(text_bounds)) {
            return false;
        }

        const float cell = font_size / 7.0F;
        const float advance = cell * 6.0F;
        if (!std::isfinite(cell) || !std::isfinite(advance) || cell <= 0.0F)
            return false;

        std::vector<UiGlyphQuad> generated;
        for (std::size_t index = 0; index < text.size(); ++index) {
            const auto rows = pattern(static_cast<unsigned char>(text[index]));
            const float origin_x = text_bounds.x +
                static_cast<float>(index) * advance;
            if (!std::isfinite(origin_x)) return false;

            for (std::size_t row = 0; row < rows.size(); ++row) {
                std::size_t column = 0U;
                while (column < 5U) {
                    const auto is_set = [&](std::size_t col) {
                        return (rows[row] &
                            static_cast<std::uint8_t>(1U << (4U - col))) != 0U;
                    };
                    while (column < 5U && !is_set(column)) ++column;
                    const std::size_t run_begin = column;
                    while (column < 5U && is_set(column)) ++column;
                    if (run_begin == column) continue;
                    if (generated.size() >= maximum_quads) return false;

                    const float x = origin_x +
                        static_cast<float>(run_begin) * cell;
                    const float y = text_bounds.y +
                        static_cast<float>(row) * cell;
                    const float width =
                        static_cast<float>(column - run_begin) * cell;
                    if (!std::isfinite(x) || !std::isfinite(y) ||
                        !std::isfinite(width)) return false;
                    generated.push_back({
                        {x, y, width, cell},
                        {0.0F, 0.0F, 1.0F, 1.0F},
                        0U
                    });
                }
            }
        }

        output.swap(generated);
        return true;
    }

private:
    [[nodiscard]] static bool finite(UiRect rect) noexcept {
        return std::isfinite(rect.x) && std::isfinite(rect.y) &&
            std::isfinite(rect.width) && std::isfinite(rect.height) &&
            rect.width >= 0.0F && rect.height >= 0.0F;
    }

    [[nodiscard]] static std::array<std::uint8_t, 7> pattern(
        unsigned char character) noexcept {
        if (character >= static_cast<unsigned char>('a') &&
            character <= static_cast<unsigned char>('z')) {
            character = static_cast<unsigned char>(
                character - static_cast<unsigned char>('a') +
                static_cast<unsigned char>('A'));
        }
        switch (character) {
        case 'A': return {14,17,17,31,17,17,17};
        case 'B': return {30,17,17,30,17,17,30};
        case 'C': return {14,17,16,16,16,17,14};
        case 'D': return {30,17,17,17,17,17,30};
        case 'E': return {31,16,16,30,16,16,31};
        case 'F': return {31,16,16,30,16,16,16};
        case 'G': return {14,17,16,23,17,17,15};
        case 'H': return {17,17,17,31,17,17,17};
        case 'I': return {14,4,4,4,4,4,14};
        case 'J': return {7,2,2,2,18,18,12};
        case 'K': return {17,18,20,24,20,18,17};
        case 'L': return {16,16,16,16,16,16,31};
        case 'M': return {17,27,21,21,17,17,17};
        case 'N': return {17,25,21,19,17,17,17};
        case 'O': return {14,17,17,17,17,17,14};
        case 'P': return {30,17,17,30,16,16,16};
        case 'Q': return {14,17,17,17,21,18,13};
        case 'R': return {30,17,17,30,20,18,17};
        case 'S': return {15,16,16,14,1,1,30};
        case 'T': return {31,4,4,4,4,4,4};
        case 'U': return {17,17,17,17,17,17,14};
        case 'V': return {17,17,17,17,17,10,4};
        case 'W': return {17,17,17,21,21,21,10};
        case 'X': return {17,17,10,4,10,17,17};
        case 'Y': return {17,17,10,4,4,4,4};
        case 'Z': return {31,1,2,4,8,16,31};
        case '0': return {14,17,19,21,25,17,14};
        case '1': return {4,12,4,4,4,4,14};
        case '2': return {14,17,1,2,4,8,31};
        case '3': return {30,1,1,14,1,1,30};
        case '4': return {2,6,10,18,31,2,2};
        case '5': return {31,16,16,30,1,1,30};
        case '6': return {14,16,16,30,17,17,14};
        case '7': return {31,1,2,4,8,8,8};
        case '8': return {14,17,17,14,17,17,14};
        case '9': return {14,17,17,15,1,1,14};
        case '!': return {4,4,4,4,4,0,4};
        case '?': return {14,17,1,2,4,0,4};
        case '.': return {0,0,0,0,0,0,4};
        case ',': return {0,0,0,0,0,4,8};
        case ':': return {0,4,0,0,4,0,0};
        case '-': return {0,0,0,31,0,0,0};
        case '+': return {0,4,4,31,4,4,0};
        case '/': return {1,1,2,4,8,16,16};
        case '(': return {2,4,8,8,8,4,2};
        case ')': return {8,4,2,2,2,4,8};
        case '\'': return {4,4,2,0,0,0,0};
        case ' ': return {0,0,0,0,0,0,0};
        default: return {14,17,1,2,4,0,4};
        }
    }
};

} // namespace dream::ui
