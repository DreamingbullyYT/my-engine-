#include <dream/render/bgfx/bgfx_clear_backend.hpp>

#include <bgfx/bgfx.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <map>
#include <string>
#include <utility>
#include <vector>

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

// bgfx clear color uses packed 0xRRGGBBAA.
[[nodiscard]] std::uint32_t pack_clear_rgba(Color color) noexcept {
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

[[nodiscard]] std::size_t shader_format_index(
    Bgfx2DShaderFormat format) noexcept {
    switch (format) {
    case Bgfx2DShaderFormat::OpenGLES: return 0U;
    case Bgfx2DShaderFormat::OpenGL: return 1U;
    case Bgfx2DShaderFormat::Vulkan: return 2U;
    }
    return 3U;
}

[[nodiscard]] bool renderer_shader_format(
    bgfx::RendererType::Enum renderer, Bgfx2DShaderFormat& format) noexcept {
    switch (renderer) {
    case bgfx::RendererType::OpenGLES:
        format = Bgfx2DShaderFormat::OpenGLES;
        return true;
    case bgfx::RendererType::OpenGL:
        format = Bgfx2DShaderFormat::OpenGL;
        return true;
    case bgfx::RendererType::Vulkan:
        format = Bgfx2DShaderFormat::Vulkan;
        return true;
    default:
        return false;
    }
}

struct PackedVertex2D final {
    float x;
    float y;
    float u;
    float v;
    std::uint8_t color[4];
};

static_assert(sizeof(PackedVertex2D) == sizeof(float) * 4U + 4U);

[[nodiscard]] PackedVertex2D pack_vertex(const Vertex2D& vertex) noexcept {
    return {
        vertex.position.x,
        vertex.position.y,
        vertex.uv.x,
        vertex.uv.y,
        {
            to_channel(vertex.color.r),
            to_channel(vertex.color.g),
            to_channel(vertex.color.b),
            to_channel(vertex.color.a)
        }
    };
}

[[nodiscard]] std::uint64_t state_for_blend(BlendMode blend) noexcept {
    std::uint64_t state =
        BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_MSAA;
    switch (blend) {
    case BlendMode::Opaque:
        state |= BGFX_STATE_BLEND_FUNC(
            BGFX_STATE_BLEND_ONE, BGFX_STATE_BLEND_ZERO);
        break;
    case BlendMode::Alpha:
        state |= BGFX_STATE_BLEND_ALPHA;
        break;
    case BlendMode::Additive:
        state |= BGFX_STATE_BLEND_FUNC(
            BGFX_STATE_BLEND_ONE, BGFX_STATE_BLEND_ONE);
        break;
    }
    return state;
}

[[nodiscard]] world::Matrix4x4 lift_2d_matrix(
    const world::Matrix3x3& matrix) noexcept {
    world::Matrix4x4 result;
    result.at(0, 0) = matrix.at(0, 0);
    result.at(0, 1) = matrix.at(0, 1);
    result.at(0, 2) = 0.0F;
    result.at(0, 3) = matrix.at(0, 2);
    result.at(1, 0) = matrix.at(1, 0);
    result.at(1, 1) = matrix.at(1, 1);
    result.at(1, 2) = 0.0F;
    result.at(1, 3) = matrix.at(1, 2);
    result.at(2, 0) = 0.0F;
    result.at(2, 1) = 0.0F;
    result.at(2, 2) = 1.0F;
    result.at(2, 3) = 0.0F;
    result.at(3, 0) = 0.0F;
    result.at(3, 1) = 0.0F;
    result.at(3, 2) = 0.0F;
    result.at(3, 3) = 1.0F;
    return result;
}

} // namespace

struct BgfxClearBackend::Impl final {
    struct ShaderBinary final {
        std::vector<std::byte> vertex;
        std::vector<std::byte> fragment;
    };

    RenderSurfaceInfo surface{};
    platform::NativeSurface native_surface{};
    std::array<ShaderBinary, 3> shader_binaries{};
    std::map<RenderAssetId, bgfx::TextureHandle> textures;
    bgfx::TextureHandle white_texture{};
    bgfx::ProgramHandle sprite_program{};
    bgfx::UniformHandle tint{};
    bgfx::UniformHandle sampler{};
    bgfx::VertexLayout vertex_layout{};
    std::uint16_t maximum_texture_dimension{0};
    bool supports_2d{false};
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

bool BgfxClearBackend::set_2d_shader_binaries(
    Bgfx2DShaderFormat format,
    std::span<const std::byte> vertex_shader,
    std::span<const std::byte> fragment_shader,
    std::string& error) {
    if (!impl_ || impl_->initialized) {
        error = "2D shader binaries must be installed before backend initialization";
        return false;
    }
    const auto index = shader_format_index(format);
    if (index >= impl_->shader_binaries.size() ||
        vertex_shader.empty() || fragment_shader.empty() ||
        vertex_shader.size() > std::numeric_limits<std::uint32_t>::max() ||
        fragment_shader.size() > std::numeric_limits<std::uint32_t>::max()) {
        error = "2D shader binaries are empty or exceed bgfx memory limits";
        return false;
    }

    auto& destination = impl_->shader_binaries[index];
    destination.vertex.assign(vertex_shader.begin(), vertex_shader.end());
    destination.fragment.assign(fragment_shader.begin(), fragment_shader.end());
    return true;
}

bool BgfxClearBackend::create_texture_rgba8(
    RenderAssetId texture_id, Extent2D extent,
    std::span<const std::byte> pixels, std::string& error) {
    if (!initialized()) {
        error = "bgfx backend is not initialized";
        return false;
    }
    if (!impl_->supports_2d) {
        error = "2D shader program is unavailable for the selected bgfx renderer";
        return false;
    }
    if (texture_id == 0U || !valid_extent(extent) ||
        extent.width > impl_->maximum_texture_dimension ||
        extent.height > impl_->maximum_texture_dimension ||
        extent.width > std::numeric_limits<std::size_t>::max() / 4U / extent.height) {
        error = "Texture ID or texture dimensions are invalid";
        return false;
    }
    const std::size_t expected_bytes =
        static_cast<std::size_t>(extent.width) *
        static_cast<std::size_t>(extent.height) * 4U;
    if (pixels.size() != expected_bytes ||
        pixels.size() > std::numeric_limits<std::uint32_t>::max()) {
        error = "RGBA8 pixel payload size does not match texture dimensions";
        return false;
    }
    if (impl_->textures.contains(texture_id)) {
        error = "Texture ID is already registered";
        return false;
    }

    const auto* memory = bgfx::copy(
        pixels.data(), static_cast<std::uint32_t>(pixels.size()));
    const auto handle = bgfx::createTexture2D(
        static_cast<std::uint16_t>(extent.width),
        static_cast<std::uint16_t>(extent.height),
        false, 1U, bgfx::TextureFormat::RGBA8,
        BGFX_TEXTURE_NONE | BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP,
        memory);
    if (!bgfx::isValid(handle)) {
        error = "bgfx failed to create the requested RGBA8 texture";
        return false;
    }

    impl_->textures.emplace(texture_id, handle);
    return true;
}

bool BgfxClearBackend::destroy_texture(
    RenderAssetId texture_id, std::string& error) {
    if (!initialized()) {
        error = "bgfx backend is not initialized";
        return false;
    }
    const auto found = impl_->textures.find(texture_id);
    if (found == impl_->textures.end()) {
        error = "Texture ID is not registered";
        return false;
    }
    bgfx::destroy(found->second);
    impl_->textures.erase(found);
    return true;
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

    const auto has_shader_format = [&](Bgfx2DShaderFormat format) {
        const auto& shaders = impl_->shader_binaries[shader_format_index(format)];
        return !shaders.vertex.empty() && !shaders.fragment.empty();
    };

    bgfx::Init init;
    init.type = bgfx::RendererType::Count;
    if (native_surface.kind == platform::NativeSurfaceKind::Android) {
        if (has_shader_format(Bgfx2DShaderFormat::Vulkan))
            init.type = bgfx::RendererType::Vulkan;
        else if (has_shader_format(Bgfx2DShaderFormat::OpenGLES))
            init.type = bgfx::RendererType::OpenGLES;
    } else if (native_surface.kind == platform::NativeSurfaceKind::X11 ||
               native_surface.kind == platform::NativeSurfaceKind::Wayland) {
        if (has_shader_format(Bgfx2DShaderFormat::Vulkan))
            init.type = bgfx::RendererType::Vulkan;
        else if (has_shader_format(Bgfx2DShaderFormat::OpenGL))
            init.type = bgfx::RendererType::OpenGL;
    }
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

    const auto* caps = bgfx::getCaps();
    impl_->maximum_texture_dimension =
        caps != nullptr ? caps->limits.maxTextureSize : 0U;

    const std::array<std::byte, 4> white_pixel{
        std::byte{0xFF}, std::byte{0xFF}, std::byte{0xFF}, std::byte{0xFF}
    };
    impl_->white_texture = bgfx::createTexture2D(
        1U, 1U, false, 1U, bgfx::TextureFormat::RGBA8,
        BGFX_TEXTURE_NONE | BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP,
        bgfx::copy(white_pixel.data(), static_cast<std::uint32_t>(white_pixel.size())));
    if (!bgfx::isValid(impl_->white_texture)) {
        error = "bgfx could not allocate its default white texture";
        shutdown();
        return false;
    }

    Bgfx2DShaderFormat shader_format{};
    if (renderer_shader_format(bgfx::getRendererType(), shader_format)) {
        const auto shader_index = shader_format_index(shader_format);
        const auto& shaders = impl_->shader_binaries[shader_index];
        if (!shaders.vertex.empty() && !shaders.fragment.empty()) {
            const auto vertex_shader = bgfx::createShader(bgfx::copy(
                shaders.vertex.data(),
                static_cast<std::uint32_t>(shaders.vertex.size())));
            const auto fragment_shader = bgfx::createShader(bgfx::copy(
                shaders.fragment.data(),
                static_cast<std::uint32_t>(shaders.fragment.size())));
            if (!bgfx::isValid(vertex_shader) || !bgfx::isValid(fragment_shader)) {
                if (bgfx::isValid(vertex_shader)) bgfx::destroy(vertex_shader);
                if (bgfx::isValid(fragment_shader)) bgfx::destroy(fragment_shader);
                error = "bgfx rejected the supplied 2D vertex or fragment shader";
                shutdown();
                return false;
            }

            impl_->sprite_program =
                bgfx::createProgram(vertex_shader, fragment_shader, true);
            if (!bgfx::isValid(impl_->sprite_program)) {
                error = "bgfx could not link the 2D shader program";
                shutdown();
                return false;
            }

            impl_->tint = bgfx::createUniform("u_tint", bgfx::UniformType::Vec4);
            impl_->sampler =
                bgfx::createUniform("s_texColor", bgfx::UniformType::Sampler);
            if (!bgfx::isValid(impl_->tint) ||
                !bgfx::isValid(impl_->sampler)) {
                error = "bgfx could not create the required 2D shader uniforms";
                shutdown();
                return false;
            }

            impl_->vertex_layout
                .begin(bgfx::getRendererType())
                .add(bgfx::Attrib::Position, 2U, bgfx::AttribType::Float)
                .add(bgfx::Attrib::TexCoord0, 2U, bgfx::AttribType::Float)
                .add(bgfx::Attrib::Color0, 4U, bgfx::AttribType::Uint8, true)
                .end();
            impl_->supports_2d = true;
        }
    }

    capabilities = {};
    capabilities.supports_2d = impl_->supports_2d;
    capabilities.supports_3d = false;
    capabilities.supports_instancing = false;
    capabilities.maximum_draws_per_frame =
        impl_->supports_2d ? 100000U : 0U;
    capabilities.maximum_texture_dimension = impl_->maximum_texture_dimension;
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
    if (!frame.draws_3d.empty()) {
        error = "bgfx backend does not yet support 3D mesh commands";
        return false;
    }
    if (!frame.draws_2d.empty() && !impl_->supports_2d) {
        error = "no compatible offline 2D shader binary was supplied for this GPU renderer";
        return false;
    }

    std::vector<bgfx::TextureHandle> textures;
    textures.reserve(frame.draws_2d.size());
    for (const auto& draw : frame.draws_2d) {
        if (draw.texture_id == 0U) {
            textures.push_back(impl_->white_texture);
            continue;
        }
        const auto found = impl_->textures.find(draw.texture_id);
        if (found == impl_->textures.end()) {
            error = "2D draw references an unregistered texture asset ID";
            return false;
        }
        textures.push_back(found->second);
    }

    bgfx::setViewRect(
        0U, 0U, 0U,
        static_cast<std::uint16_t>(frame.framebuffer_extent.width),
        static_cast<std::uint16_t>(frame.framebuffer_extent.height));
    if (!frame.camera_2d && !frame.draws_2d.empty()) {
        error = "2D draw packet is missing its camera matrices";
        return false;
    }

    std::uint16_t clear_flags = BGFX_CLEAR_NONE;
    if (frame.clear_color_buffer) clear_flags |= BGFX_CLEAR_COLOR;
    if (frame.clear_depth_buffer) clear_flags |= BGFX_CLEAR_DEPTH;
    bgfx::setViewClear(
        0U, clear_flags, pack_clear_rgba(frame.clear_color), 1.0F, 0U);

    if (frame.draws_2d.empty()) {
        bgfx::touch(0U);
        (void)bgfx::frame();
        return true;
    }

    const auto view_matrix = lift_2d_matrix(frame.camera_2d->view);
    const auto projection_matrix = lift_2d_matrix(frame.camera_2d->projection);
    bgfx::setViewTransform(
        0U, view_matrix.values.data(), projection_matrix.values.data());

    if (frame.vertices_2d.size() > std::numeric_limits<std::uint32_t>::max() ||
        frame.indices_2d.size() > std::numeric_limits<std::uint32_t>::max()) {
        error = "2D transient vertex or index count exceeds bgfx limits";
        return false;
    }

    bgfx::TransientVertexBuffer vertex_buffer{};
    bgfx::TransientIndexBuffer index_buffer{};
    const auto vertex_count = static_cast<std::uint32_t>(frame.vertices_2d.size());
    const auto index_count = static_cast<std::uint32_t>(frame.indices_2d.size());
    if (!bgfx::allocTransientBuffers(
            &vertex_buffer, impl_->vertex_layout, vertex_count,
            &index_buffer, index_count, true)) {
        error = "bgfx transient vertex/index buffers are exhausted for this frame";
        return false;
    }

    auto* packed_vertices =
        reinterpret_cast<PackedVertex2D*>(vertex_buffer.data);
    for (std::size_t index = 0; index < frame.vertices_2d.size(); ++index)
        packed_vertices[index] = pack_vertex(frame.vertices_2d[index]);

    if (!frame.indices_2d.empty()) {
        std::memcpy(
            index_buffer.data,
            frame.indices_2d.data(),
            frame.indices_2d.size() * sizeof(std::uint32_t));
    }

    constexpr bgfx::ViewId main_view = 0U;
    for (std::size_t index = 0; index < frame.draws_2d.size(); ++index) {
        const auto& draw = frame.draws_2d[index];
        const auto model = lift_2d_matrix(draw.transform);
        const std::array<float, 4> tint{
            draw.tint.r, draw.tint.g, draw.tint.b, draw.tint.a
        };

        if (draw.base_vertex > vertex_count) {
            error = "2D draw base vertex exceeds the uploaded vertex buffer";
            return false;
        }
        bgfx::setTransform(model.values.data());
        bgfx::setVertexBuffer(
            0U, &vertex_buffer, draw.base_vertex,
            vertex_count - draw.base_vertex);
        bgfx::setIndexBuffer(
            &index_buffer, draw.first_index, draw.index_count);
        bgfx::setUniform(impl_->tint, tint.data(), 1U);
        bgfx::setTexture(0U, impl_->sampler, textures[index]);
        bgfx::setState(state_for_blend(draw.blend));
        bgfx::submit(main_view, impl_->sprite_program);
    }

    (void)bgfx::frame();
    return true;
}

void BgfxClearBackend::shutdown() noexcept {
    if (!impl_ || !impl_->owns_global_context) return;

    if (impl_->initialized) {
        for (const auto& [id, texture] : impl_->textures) {
            (void)id;
            bgfx::destroy(texture);
        }
        impl_->textures.clear();

        if (bgfx::isValid(impl_->white_texture))
            bgfx::destroy(impl_->white_texture);
        if (bgfx::isValid(impl_->sprite_program))
            bgfx::destroy(impl_->sprite_program);
        if (bgfx::isValid(impl_->tint))
            bgfx::destroy(impl_->tint);
        if (bgfx::isValid(impl_->sampler))
            bgfx::destroy(impl_->sampler);
        bgfx::shutdown();
    }

    impl_->white_texture = BGFX_INVALID_HANDLE;
    impl_->sprite_program = BGFX_INVALID_HANDLE;
    impl_->tint = BGFX_INVALID_HANDLE;
    impl_->sampler = BGFX_INVALID_HANDLE;
    impl_->maximum_texture_dimension = 0U;
    impl_->supports_2d = false;
    impl_->initialized = false;
    impl_->owns_global_context = false;
    impl_->surface = {};
    impl_->native_surface = {};
    bgfx_context_guard().clear(std::memory_order_release);
}

} // namespace dream::render
