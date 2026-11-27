#pragma once

#include <dream/render/render_backend.hpp>

#include <memory>

namespace dream::render {

// Initial wgpu-native Android backend. This milestone intentionally exposes
// clear/present only; 2D and 3D capabilities stay disabled until their WGSL
// pipelines and GPU resource lifetime are implemented.
class WgpuNativeBackend final : public IRenderBackend {
public:
    WgpuNativeBackend();
    ~WgpuNativeBackend() override;

    WgpuNativeBackend(const WgpuNativeBackend&) = delete;
    WgpuNativeBackend& operator=(const WgpuNativeBackend&) = delete;

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
    struct State;
    std::unique_ptr<State> state_;
};

} // namespace dream::render
