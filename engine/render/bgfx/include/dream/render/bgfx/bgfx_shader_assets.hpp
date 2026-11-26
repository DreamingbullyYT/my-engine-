#pragma once

#include <dream/platform/filesystem.hpp>
#include <dream/render/bgfx/bgfx_clear_backend.hpp>

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace dream::render {

struct BgfxShaderAssetLimits final {
    std::size_t maximum_vertex_shader_bytes{4U * 1024U * 1024U};
    std::size_t maximum_fragment_shader_bytes{4U * 1024U * 1024U};
};

// Loads a precompiled shader pair into an uninitialized bgfx backend. Both
// assets are read into temporary buffers first, so a failed second read never
// leaves the backend with a partially replaced shader pair. Shader binaries
// must be produced by the shaderc version pinned by this repository.
template <typename Backend>
[[nodiscard]] bool load_bgfx_2d_shader_binaries(
    Backend& backend,
    const platform::IFileSystem& file_system,
    Bgfx2DShaderFormat format,
    std::string_view vertex_shader_path,
    std::string_view fragment_shader_path,
    std::string& error,
    BgfxShaderAssetLimits limits = {}) {
    if (limits.maximum_vertex_shader_bytes == 0U ||
        limits.maximum_fragment_shader_bytes == 0U) {
        error = "bgfx shader asset size limits must be non-zero";
        return false;
    }

    std::vector<std::byte> vertex_shader;
    std::vector<std::byte> fragment_shader;
    platform::FileError file_error = platform::FileError::None;

    if (!file_system.read_file(
            vertex_shader_path, vertex_shader,
            limits.maximum_vertex_shader_bytes, file_error)) {
        error = "Unable to load bgfx vertex shader asset: " +
                std::string(vertex_shader_path) + " (error " +
                std::to_string(static_cast<unsigned>(file_error)) + ")";
        return false;
    }
    if (vertex_shader.empty()) {
        error = "bgfx vertex shader asset is empty: " +
                std::string(vertex_shader_path);
        return false;
    }

    file_error = platform::FileError::None;
    if (!file_system.read_file(
            fragment_shader_path, fragment_shader,
            limits.maximum_fragment_shader_bytes, file_error)) {
        error = "Unable to load bgfx fragment shader asset: " +
                std::string(fragment_shader_path) + " (error " +
                std::to_string(static_cast<unsigned>(file_error)) + ")";
        return false;
    }
    if (fragment_shader.empty()) {
        error = "bgfx fragment shader asset is empty: " +
                std::string(fragment_shader_path);
        return false;
    }

    if (!backend.set_2d_shader_binaries(
            format,
            std::span<const std::byte>{vertex_shader},
            std::span<const std::byte>{fragment_shader},
            error)) {
        return false;
    }

    error.clear();
    return true;
}

} // namespace dream::render
