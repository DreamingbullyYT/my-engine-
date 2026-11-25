#include <dream/core/handle.hpp>

#include <cstdlib>
#include <iostream>
#include <type_traits>
#include <string_view>

namespace {

void require(bool condition, std::string_view message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\\n';
        std::exit(EXIT_FAILURE);
    }
}

struct TextureTag;
struct BufferTag;

} // namespace

int main() {
    using dream::core::Handle;
    using dream::core::HandlePool;

    static_assert(!std::is_convertible_v<Handle<TextureTag>, Handle<BufferTag>>);
    static_assert(!std::is_same_v<Handle<TextureTag>, Handle<BufferTag>>);

    HandlePool<TextureTag> textures;
    const auto invalid = Handle<TextureTag>{};
    require(!invalid, "default handle is invalid");
    require(!textures.is_alive(invalid), "invalid handle is not alive");
    require(!textures.release(invalid), "invalid handle cannot be released");

    const auto first = textures.allocate();
    require(first, "first allocation returns a valid handle");
    require(first.index() == 0 && first.generation() == 1, "first handle identity");
    require(textures.is_alive(first), "new handle is alive");
    require(textures.capacity() == 1, "capacity tracks allocated slots");

    const auto second = textures.allocate();
    require(second && second.index() == 1, "second allocation uses a new slot");

    require(textures.release(first), "live handle releases");
    require(!textures.is_alive(first), "released handle becomes stale");
    require(!textures.release(first), "double release is rejected");

    const auto reused = textures.allocate();
    require(reused.index() == first.index(), "released slot is reused");
    require(reused.generation() != first.generation(), "reused slot gets a new generation");
    require(textures.is_alive(reused), "reused handle is alive");
    require(!textures.is_alive(first), "old generation stays stale");

    HandlePool<BufferTag> buffers;
    const auto buffer = buffers.allocate();
    require(buffer.index() == reused.index(), "separate pools have independent indices");
    require(!textures.is_alive(Handle<TextureTag>{buffer.index(), buffer.generation()}),
            "identity from another pool is not accidentally alive unless generations coincide");

    require(textures.release(second), "second handle releases");
    require(textures.release(reused), "reused handle releases");

    std::cout << "Dream Engine handle tests passed.\\n";
    return EXIT_SUCCESS;
}
