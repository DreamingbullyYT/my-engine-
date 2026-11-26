#include <dream/assets/asset_cache.hpp>

#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>

namespace {
void require(bool condition, std::string_view message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}
struct TextureTag;
struct Texture final {
    explicit Texture(int id) : id(id) {}
    int id;
};
}

int main() {
    using namespace dream::assets;
    AssetCache<Texture, TextureTag> cache(100);

    const auto a = cache.put("texture/a", 40, 1);
    const auto b = cache.put("texture/b", 40, 2);
    require(a.inserted() && b.inserted(), "assets are inserted");
    require(cache.used_bytes() == 80 && cache.size() == 2, "usage is tracked");

    bool read_ok = false;
    require(cache.with_resource("texture/a", [&](Texture& texture) {
        read_ok = texture.id == 1;
    }), "resource can be accessed by key");
    require(read_ok, "resource content is preserved");

    const auto c = cache.put("texture/c", 50, 3);
    require(c.inserted(), "new asset is inserted after LRU eviction");
    require(!cache.contains("texture/b"), "least recently used asset is evicted");
    require(cache.contains("texture/a") && cache.contains("texture/c"),
            "recently used and new assets remain");
    require(cache.used_bytes() == 90 && cache.size() == 2, "eviction updates accounting");

    require(cache.put("texture/a", 1, 4).status == AssetCachePutStatus::DuplicateKey,
            "duplicate key is rejected");
    require(cache.put("", 1, 4).status == AssetCachePutStatus::InvalidKey,
            "empty key is rejected");
    require(cache.put("oversized", 101, 4).status == AssetCachePutStatus::TooLarge,
            "asset larger than budget is rejected");

    require(!cache.with_resource("missing", [](Texture&) {}),
            "missing resource cannot be accessed");
    require(cache.erase("texture/a"), "explicit erase succeeds");
    require(!cache.contains("texture/a"), "erased resource is absent");
    require(cache.used_bytes() == 50, "explicit erase updates accounting");

    cache.set_byte_budget(20);
    require(cache.size() == 0 && cache.used_bytes() == 0,
            "shrinking budget evicts until within budget");
    require(cache.byte_budget() == 20, "new budget is reported");

    cache.set_byte_budget(0);
    require(cache.put("zero", 0, 9).inserted(), "zero-cost resource is permitted");
    require(cache.used_bytes() == 0, "zero-cost resource does not consume budget");

    std::cout << "Dream Engine asset cache tests passed.\n";
    return EXIT_SUCCESS;
}
