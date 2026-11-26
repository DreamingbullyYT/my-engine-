#pragma once

#include <dream/render/render_backend.hpp>

#include <cstddef>
#include <memory>
#include <span>

namespace dream::render {

enum class Bgfx2DShaderFormat : std::uint8_t {
    OpenGLES,
    OpenGL,
    Vulkan
};

// Shader binaries are generated offline with the matching pinned bgfx shaderc.
// The backend copies these bytes before initialization; callers may release
// their asset buffers after set_2d_shader_binaries returns.
class BgfxClearBackend final : public IRenderBackend {
public:
    BgfxClearBackend();
    ~BgfxClearBackend() override;

    BgfxClearBackend(const BgfxClearBackend&) = delete;
    BgfxClearBackend& operator=(const BgfxClearBackend&) = delete;

    [[nodiscard]] bool initialized() const noexcept;

    [[nodiscard]] bool set_2d_shader_binaries(
        Bgfx2DShaderFormat format,
        std::span<const std::byte> vertex_shader,
        std::span<const std::byte> fragment_shader,
        std::string& error);

    [[nodiscard]] bool create_texture_rgba8(
        RenderAssetId texture_id, Extent2D extent,
        std::span<const std::byte> pixels, std::string& error) override;

    [[nodiscard]] bool destroy_texture(
        RenderAssetId texture_id, std::string& error) override;

    [[nodiscard]] bool initialize(
        platform::NativeSurface native_surface,
        const RenderSurfaceInfo& surface,
        RenderCapabilities& capabilities,
        std::string& error) override;

    [[nodiscard]] bool resize(
        const RenderSurfaceInfo& surface, std::string& error) override;

    [[nodiscard]] bool render(
        const RenderFramePacket& frame, std::string& error) override;

    void shutdown() noexcept override;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace dream::render
