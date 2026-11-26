#include <dream/render/bgfx/bgfx_clear_backend.hpp>

#include <type_traits>

static_assert(std::is_base_of_v<dream::render::IRenderBackend,
                                dream::render::BgfxClearBackend>);

int main() {
    dream::render::BgfxClearBackend backend;
    return backend.initialized() ? 1 : 0;
}
