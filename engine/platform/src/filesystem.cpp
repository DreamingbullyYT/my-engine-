#include <dream/platform/filesystem.hpp>

namespace dream::platform {
namespace {

bool valid_utf8(std::string_view text) noexcept {
    std::size_t i = 0;
    while (i < text.size()) {
        const auto first = static_cast<unsigned char>(text[i]);
        if (first <= 0x7f) {
            ++i;
            continue;
        }

        std::uint32_t codepoint = 0;
        std::size_t continuation_count = 0;
        if ((first & 0xe0u) == 0xc0u) {
            codepoint = first & 0x1fu;
            continuation_count = 1;
            if (codepoint < 2) return false;
        } else if ((first & 0xf0u) == 0xe0u) {
            codepoint = first & 0x0fu;
            continuation_count = 2;
        } else if ((first & 0xf8u) == 0xf0u) {
            codepoint = first & 0x07u;
            continuation_count = 3;
        } else {
            return false;
        }

        if (i + continuation_count >= text.size()) return false;
        for (std::size_t j = 1; j <= continuation_count; ++j) {
            const auto next = static_cast<unsigned char>(text[i + j]);
            if ((next & 0xc0u) != 0x80u) return false;
            codepoint = (codepoint << 6u) | (next & 0x3fu);
        }

        if ((continuation_count == 2 && codepoint < 0x800u) ||
            (continuation_count == 3 && codepoint < 0x10000u) ||
            codepoint > 0x10ffffu ||
            (codepoint >= 0xd800u && codepoint <= 0xdfffu)) {
            return false;
        }
        i += continuation_count + 1;
    }
    return true;
}

} // namespace

bool is_valid_virtual_path(std::string_view path) noexcept {
    if (path.empty() || path.size() > 4096 || path.front() == '/' ||
        path.find('\\\\') != std::string_view::npos ||
        path.find(':') != std::string_view::npos ||
        path.find('\0') != std::string_view::npos ||
        !valid_utf8(path)) {
        return false;
    }

    for (const unsigned char character : path) {
        if (character < 0x20u || character == 0x7fu ||
            character == '<' || character == '>' || character == '"' ||
            character == '|' || character == '?' || character == '*') {
            return false;
        }
    }

    std::size_t segment_start = 0;
    while (segment_start < path.size()) {
        const auto separator = path.find('/', segment_start);
        const auto segment_end = separator == std::string_view::npos
            ? path.size()
            : separator;
        const auto segment = path.substr(segment_start, segment_end - segment_start);
        if (segment.empty() || segment == "." || segment == ".." ||
            segment.back() == '.' || segment.back() == ' ') {
            return false;
        }
        segment_start = segment_end + 1;
    }

    return path.back() != '/';
}

} // namespace dream::platform
