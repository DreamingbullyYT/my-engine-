#include <dream/render/bgfx/bgfx_shader_assets.hpp>

#include <cstdlib>
#include <iostream>
#include <map>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace {
using namespace dream::platform;
using namespace dream::render;

void require(bool ok, std::string_view message) {
    if (!ok) {
        std::cerr << "FAILED: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class MemoryFileSystem final : public IFileSystem {
public:
    std::map<std::string,std::vector<std::byte>> files;

    bool read_file(std::string_view path,std::vector<std::byte>& output,
                   std::size_t max_bytes,FileError& error) const override {
        const auto it=files.find(std::string(path));
        if(it==files.end()){error=FileError::NotFound;return false;}
        if(it->second.size()>max_bytes){error=FileError::TooLarge;return false;}
        output=it->second;error=FileError::None;return true;
    }
    bool write_file(std::string_view,std::span<const std::byte>,FileError& error) override {
        error=FileError::Unsupported;return false;
    }
    bool exists(std::string_view,bool& result,FileError& error) const override {
        result=false;error=FileError::Unsupported;return false;
    }
    bool create_directories(std::string_view,FileError& error) override {
        error=FileError::Unsupported;return false;
    }
    bool remove_file(std::string_view,FileError& error) override {
        error=FileError::Unsupported;return false;
    }
    bool list_directory(std::string_view,std::vector<DirectoryEntry>&,
                        FileError& error) const override {
        error=FileError::Unsupported;return false;
    }
};

class FakeBackend final {
public:
    bool accept{true};
    std::size_t calls{0};
    Bgfx2DShaderFormat format{Bgfx2DShaderFormat::OpenGLES};
    std::vector<std::byte> vertex;
    std::vector<std::byte> fragment;

    bool set_2d_shader_binaries(Bgfx2DShaderFormat requested,
        std::span<const std::byte> vs,std::span<const std::byte> fs,
        std::string& error) {
        ++calls;
        if(!accept){error="backend rejected shaders";return false;}
        format=requested;
        vertex.assign(vs.begin(),vs.end());
        fragment.assign(fs.begin(),fs.end());
        error.clear();
        return true;
    }
};
}

int main() {
    MemoryFileSystem files;
    files.files["shader/vs.bin"]={std::byte{1},std::byte{2},std::byte{3}};
    files.files["shader/fs.bin"]={std::byte{4},std::byte{5}};
    FakeBackend backend;
    std::string error="stale error";
    require(load_bgfx_2d_shader_binaries(backend,files,
        Bgfx2DShaderFormat::OpenGLES,"shader/vs.bin","shader/fs.bin",error),
        "both precompiled shader assets load");
    require(backend.calls==1&&backend.format==Bgfx2DShaderFormat::OpenGLES,
        "backend receives the selected renderer format exactly once");
    require(backend.vertex.size()==3&&backend.fragment.size()==2&&error.empty(),
        "complete binary payloads are passed to the backend");

    const auto old_vertex=backend.vertex;
    const auto old_fragment=backend.fragment;
    require(!load_bgfx_2d_shader_binaries(backend,files,
        Bgfx2DShaderFormat::OpenGL,"shader/vs.bin","missing.bin",error),
        "missing fragment shader is reported");
    require(backend.calls==1&&backend.vertex==old_vertex&&backend.fragment==old_fragment,
        "backend is not modified when the second shader read fails");

    require(!load_bgfx_2d_shader_binaries(backend,files,
        Bgfx2DShaderFormat::OpenGLES,"shader/vs.bin","shader/fs.bin",error,
        BgfxShaderAssetLimits{2,4}),
        "shader file size limit is enforced by the asset filesystem");
    require(backend.calls==1,"oversized shader assets are not installed");

    files.files["shader/empty.bin"]={};
    require(!load_bgfx_2d_shader_binaries(backend,files,
        Bgfx2DShaderFormat::OpenGLES,"shader/empty.bin","shader/fs.bin",error),
        "empty shader asset is rejected");

    files.files.erase("shader/empty.bin");
    backend.accept=false;
    require(!load_bgfx_2d_shader_binaries(backend,files,
        Bgfx2DShaderFormat::Vulkan,"shader/vs.bin","shader/fs.bin",error),
        "backend installation error is propagated");
    require(error=="backend rejected shaders","backend error string is preserved");

    require(!load_bgfx_2d_shader_binaries(backend,files,
        Bgfx2DShaderFormat::OpenGLES,"shader/vs.bin","shader/fs.bin",error,
        BgfxShaderAssetLimits{0,4}),
        "zero size limits are rejected");
    return EXIT_SUCCESS;
}
