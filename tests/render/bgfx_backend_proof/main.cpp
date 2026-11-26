#include <bgfx/bgfx.h>

int main() {
    // This verifies the public API headers and static-library link only.
    // It intentionally does not create a GPU device or open a native window.
    const char* vulkan = bgfx::getRendererName(bgfx::RendererType::Vulkan);
    const char* gles = bgfx::getRendererName(bgfx::RendererType::OpenGLES);
    return vulkan != nullptr && gles != nullptr ? 0 : 1;
}
