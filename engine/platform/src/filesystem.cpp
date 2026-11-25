#include <dream/platform/filesystem.hpp>

namespace dream::platform {

bool is_valid_virtual_path(std::string_view path) noexcept {
    if (path.empty() || path.size() > 4096 || path.front() == '/' ||
        path.find('\\\\') != std::string_view::npos ||
        path.find(':') != std::string_view::npos ||
        path.find('\0') != std::string_view::npos) {
        return false;
    }

    std::size_t segment_start = 0;
    while (segment_start < path.size()) {
        const auto separator = path.find('/', segment_start);
        const auto segment_end = separator == std::string_view::npos
            ? path.size()
            : separator;
        const auto segment = path.substr(segment_start, segment_end - segment_start);
        if (segment.empty() || segment == "." || segment == "..") return false;
        segment_start = segment_end + 1;
    }

    return path.back() != '/';
}

} // namespace dream::platform
