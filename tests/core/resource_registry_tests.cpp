#include <dream/core/resource_registry.hpp>

#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>
#include <type_traits>

namespace {
void require(bool condition, std::string_view message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}
struct TextureTag;
struct BufferTag;
struct Resource final {
    explicit Resource(int initial) : value(initial) {}
    int value;
};
}

int main() {
    using dream::core::ResourceRegistry;
    static_assert(!std::is_same_v<dream::core::Handle<TextureTag>,
                                  dream::core::Handle<BufferTag>>);

    ResourceRegistry<Resource, TextureTag> textures;
    const auto missing = dream::core::Handle<TextureTag>{};
    require(!textures.contains(missing), "default handle is absent");
    require(!textures.destroy(missing), "default handle cannot be destroyed");

    const auto first = textures.emplace(17);
    require(static_cast<bool>(first), "resource receives a handle");
    require(textures.size() == 1, "registry counts live resources");
    require(textures.contains(first), "new resource is present");

    int observed = 0;
    require(textures.with_resource(first, [&](Resource& resource) {
        observed = resource.value;
        resource.value = 42;
    }), "live resource can be accessed");
    require(observed == 17, "callback observes constructed resource");

    require(textures.destroy(first), "resource can be destroyed");
    require(!textures.contains(first), "destroyed handle is absent");
    require(!textures.with_resource(first, [](Resource&) {}),
            "stale resource cannot be accessed");
    require(!textures.destroy(first), "double destruction is rejected");
    require(textures.size() == 0, "destroy updates live resource count");

    const auto second = textures.emplace(99);
    require(second.index() == first.index(), "released slot is reused");
    require(second.generation() != first.generation(), "reused slot gets new generation");
    require(!textures.contains(first), "old handle remains stale");
    require(textures.contains(second), "new resource is present");

    ResourceRegistry<std::string, BufferTag> buffers;
    const auto text = buffers.emplace("mesh data");
    require(buffers.with_resource(text, [&](std::string& value) {
        require(value == "mesh data", "move-aware resource construction");
    }), "independent typed registry works");

    std::cout << "Dream Engine resource registry tests passed.\n";
    return EXIT_SUCCESS;
}
