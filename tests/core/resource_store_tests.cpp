#include <dream/core/resource_store.hpp>

#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>
#include <stdexcept>

namespace {
void require(bool condition, std::string_view message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}
struct TextureTag;
struct Texture final {
    explicit Texture(int width, int height) : width(width), height(height) {}
    int width;
    int height;
};
struct FragileTag;
struct FragileResource final {
    explicit FragileResource(bool fail) {
        if (fail) throw std::runtime_error("intentional construction failure");
    }
};
}

int main() {
    dream::core::ResourceStore<FragileResource, FragileTag> fragile;
    try {
        (void)fragile.emplace("assets/retry", true);
        require(false, "resource construction failure propagates");
    } catch (const std::runtime_error&) {
        require(fragile.size() == 0, "failed construction removes placeholder key");
    }
    const auto retried = fragile.emplace("assets/retry", false);
    require(static_cast<bool>(retried), "key can be retried after construction failure");

    dream::core::ResourceStore<Texture, TextureTag> textures;

    require(!textures.contains("textures/player"), "missing key is absent");
    require(!textures.find("textures/player"), "missing key returns invalid handle");
    require(!textures.emplace("", 16, 16), "empty key is rejected");
    const std::string oversized_key(4097, 'x');
    require(!textures.emplace(oversized_key, 16, 16), "oversized key is rejected");
    require(!textures.find(oversized_key), "oversized lookup key is rejected");
    require(!textures.erase(oversized_key), "oversized erase key is rejected");

    const auto player = textures.emplace("textures/player", 64, 32);
    require(static_cast<bool>(player), "keyed resource is inserted");
    require(textures.size() == 1, "store tracks keyed resources");
    require(textures.find("textures/player") == player, "key resolves to stable handle");
    require(!textures.emplace("textures/player", 128, 128),
            "duplicate key is rejected");
    require(!textures.contains("Textures/player"), "keys are case-sensitive");

    int width = 0;
    require(textures.with_resource("textures/player", [&](Texture& texture) {
        width = texture.width;
        texture.height = 48;
    }), "resource can be accessed by key");
    require(width == 64, "key callback receives expected resource");

    require(textures.with_resource(player, [&](Texture& texture) {
        require(texture.height == 48, "resource can be accessed by handle");
    }), "resource handle access succeeds");

    require(textures.erase("textures/player"), "keyed resource can be erased");
    require(!textures.contains("textures/player"), "erased key is absent");
    require(!textures.with_resource(player, [](Texture&) {}),
            "erased handle cannot access destroyed resource");
    require(!textures.erase("textures/player"), "double erase is rejected");
    require(textures.size() == 0, "erase updates store size");

    const auto replacement = textures.emplace("textures/player", 128, 64);
    require(static_cast<bool>(replacement), "key can be reused after erase");
    require(replacement.index() == player.index(), "resource slot is reused");
    require(replacement.generation() != player.generation(),
            "replacement receives a new generation");
    require(!textures.with_resource(player, [](Texture&) {}),
            "old handle remains stale");

    std::cout << "Dream Engine resource store tests passed.\n";
    return EXIT_SUCCESS;
}
