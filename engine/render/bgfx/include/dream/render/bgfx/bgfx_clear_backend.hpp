#pragma once

#include <dream/render/render_backend.hpp>

#include <memory>

namespace dream::render {

// Minimal bgfx bootstrap. It creates and presents a cleared frame on a native
// surface, but intentionally advertises no geometry support yet. The shader,
// texture, mesh and 2D/3D pipeline implementation will build on this adapter.
class BgfxClearBackend final : public IRenderBackend {
public:
    BgfxClearBackend();
    ~BgfxClearBackend() override;

    BgfxClearBackend(const BgfxClearBackend&) = delete;
    BgfxClearBackend& operator=(const BgfxClearBackend&) = delete;

    [[nodiscard]] bool initialized() const noexcept;

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
