#include <dream/render/bgfx/bgfx_clear_backend.hpp>

#include <bgfx/bgfx.h>

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <limits>
#include <string>

namespace dream::render {
namespace {

std::atomic_flag& bgfx_context_guard() noexcept {
    static std::atomic_flag guard = ATOMIC_FLAG_INIT;
    return guard;
}

[[nodiscard]] bool valid_extent(Extent2D extent) noexcept {
    return extent.valid() &&
           extent.width <= std::numeric_limits<std::uint16_t>::max() &&
           extent.height <= std::numeric_limits<std::uint16_t>::max();
}

[[nodiscard]] std::uint8_t to_channel(float value) noexcept {
    const float clamped = std::clamp(value, 0.0F, 1.0F);
    return static_cast<std::uint8_t>(clamped * 255.0F + 0.5F);
}

[[nodiscard]] std::uint32_t pack_rgba(Color color) noexcept {
    return (static_cast<std::uint32_t>(to_channel(color.r)) << 24U) |
           (static_cast<std::uint32_t>(to_channel(color.g)) << 16U) |
           (static_cast<std::uint32_t>(to_channel(color.b)) << 8U) |
           static_cast<std::uint32_t>(to_channel(color.a));
}

[[nodiscard]] bool supported_surface(
    platform::NativeSurfaceKind kind) noexcept {
    switch (kind) {
    case platform::NativeSurfaceKind::Win32:
    case platform::NativeSurfaceKind::X11:
    case platform::NativeSurfaceKind::Wayland:
    case platform::NativeSurfaceKind::Android:
    case platform::NativeSurfaceKind::Apple:
        return true;
    case platform::NativeSurfaceKind::Unknown:
    case platform::NativeSurfaceKind::Headless:
        return false;
    }
    return false;
}

} // namespace

struct BgfxClearBackend::Impl final {
    RenderSurfaceInfo surface{};
    platform::NativeSurface native_surface{};
    bool initialized{false};
    bool owns_global_context{false};
};

BgfxClearBackend::BgfxClearBackend() : impl_(std::make_unique<Impl>()) {}

BgfxClearBackend::~BgfxClearBackend() {
    shutdown();
}

bool BgfxClearBackend::initialized() const noexcept {
    return impl_ && impl_->initialized;
}

bool BgfxClearBackend::initialize(
    platform::NativeSurface native_surface,
    const RenderSurfaceInfo& surface,
    RenderCapabilities& capabilities,
    std::string& error) {
    if (!impl_) {
        error = "bgfx backend has no implementation state";
        return false;
    }
    if (impl_->initialized) {
        error = "bgfx backend is already initialized";
        return false;
    }
    if (!supported_surface(native_surface.kind) || native_surface.window == nullptr) {
        error = "bgfx requires a supported native window surface";
        return false;
    }
    if (!valid_extent(surface.pixel_extent)) {
        error = "bgfx bootstrap requires a non-zero extent no larger than 65535x65535";
        return false;
    }
    if (bgfx_context_guard().test_and_set(std::memory_order_acquire)) {
        error = "bgfx is process-global and another BgfxClearBackend owns the context";
        return false;
    }
    impl_->owns_global_context = true;

    bgfx::Init init;
    init.type = bgfx::RendererType::Count;
    init.reset = BGFX_RESET_VSYNC;
    init.platformData.context = nullptr;
    init.platformData.queue = nullptr;
    init.platformData.type = native_surface.kind == platform::NativeSurfaceKind::Wayland
        ? bgfx::NativeWindowHandleType::Wayland
        : bgfx::NativeWindowHandleType::Default;
    init.swapChain.nwh = native_surface.window;
    init.swapChain.ndt = native_surface.display;
    init.swapChain.width = surface.pixel_extent.width;
    init.swapChain.height = surface.pixel_extent.height;

    if (!bgfx::init(init)) {
        error = "bgfx::init failed for the acquired native surface";
        impl_->owns_global_context = false;
        bgfx_context_guard().clear(std::memory_order_release);
        return false;
    }

    impl_->surface = surface;
    impl_->native_surface = native_surface;
    impl_->initialized = true;

    // This first backend stage only clears/presents. Do not claim that it can
    // render sprites or meshes until shader programs and GPU resource uploads
    // have been implemented and verified.
    capabilities = {};
    capabilities.supports_2d = false;
    capabilities.supports_3d = false;
    capabilities.supports_instancing = false;
    capabilities.maximum_draws_per_frame = 0U;
    capabilities.maximum_texture_dimension = 0U;
    return true;
}

bool BgfxClearBackend::resize(
    const RenderSurfaceInfo& surface, std::string& error) {
    if (!initialized()) {
        error = "bgfx backend is not initialized";
        return false;
    }
    if (!valid_extent(surface.pixel_extent)) {
        error = "bgfx viewport extent must be between 1 and 65535 pixels";
        return false;
    }

    bgfx::SwapChain swap_chain;
    swap_chain.nwh = impl_->native_surface.window;
    swap_chain.ndt = impl_->native_surface.display;
    swap_chain.width = surface.pixel_extent.width;
    swap_chain.height = surface.pixel_extent.height;
    bgfx::reset(BGFX_RESET_VSYNC, &swap_chain);
    impl_->surface = surface;
    return true;
}

bool BgfxClearBackend::render(
    const RenderFramePacket& frame, std::string& error) {
    if (!initialized()) {
        error = "bgfx backend is not initialized";
        return false;
    }
    if (frame.framebuffer_extent != impl_->surface.pixel_extent) {
        error = "render packet extent does not match bgfx swap chain";
        return false;
    }
    if (!frame.draws_2d.empty() || !frame.draws_3d.empty()) {
        error = "bgfx clear bootstrap does not yet support 2D or 3D draw commands";
        return false;
    }

    std::uint16_t clear_flags = BGFX_CLEAR_NONE;
    if (frame.clear_color_buffer) clear_flags |= BGFX_CLEAR_COLOR;
    if (frame.clear_depth_buffer) clear_flags |= BGFX_CLEAR_DEPTH;

    constexpr bgfx::ViewId main_view = 0U;
    bgfx::setViewRect(
        main_view, 0, 0,
        static_cast<std::uint16_t>(frame.framebuffer_extent.width),
        static_cast<std::uint16_t>(frame.framebuffer_extent.height));
    bgfx::setViewClear(
        main_view, clear_flags, pack_rgba(frame.clear_color), 1.0F, 0U);
    bgfx::touch(main_view);
    (void)bgfx::frame();
    return true;
}

void BgfxClearBackend::shutdown() noexcept {
    if (!impl_ || !impl_->owns_global_context) return;

    if (impl_->initialized) bgfx::shutdown();
    impl_->initialized = false;
    impl_->owns_global_context = false;
    impl_->surface = {};
    impl_->native_surface = {};
    bgfx_context_guard().clear(std::memory_order_release);
}

} // namespace dream::render
