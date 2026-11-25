#include <dream/dmod/manifest.hpp>
#include <dream/dmod/resolver.hpp>

#include <cstdlib>
#include <iostream>
#include <string_view>
#include <utility>
#include <vector>

namespace {

void require(bool condition, std::string_view message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << std::endl;
        std::exit(EXIT_FAILURE);
    }
}

dream::dmod::Manifest make_manifest(std::string id) {
    using namespace dream;
    dmod::Manifest manifest;
    manifest.id = std::move(id);
    manifest.name = manifest.id;
    manifest.version = {1, 0, 0};
    manifest.engine_api.minimum_inclusive = {0, 1, 0};
    manifest.entry_script = "scripts/main.dms";
    return manifest;
}

} // namespace

int main() {
    using namespace dream;

    auto valid = make_manifest("sample.core");
    require(dmod::validate_manifest(valid).valid(), "valid manifest accepted");

    auto traversal = valid;
    traversal.entry_script = "scripts/../../escape.dms";
    require(!dmod::validate_manifest(traversal).valid(), "path traversal rejected");
    auto trailing_separator = valid;
    trailing_separator.entry_script = "scripts/";
    require(!dmod::validate_manifest(trailing_separator).valid(), "trailing separator rejected");

    auto bad_id = valid;
    bad_id.id = "Sample/Bad";
    require(!dmod::validate_manifest(bad_id).valid(), "invalid ID rejected");

    auto base = make_manifest("base.lib");
    auto addon = make_manifest("addon.feature");
    addon.dependencies.push_back({"base.lib", {{1, 0, 0}, std::nullopt}, false});

    const auto ordered = dmod::resolve_load_order({addon, base}, {0, 1, 0});
    require(ordered.success(), "dependency graph resolves");
    require(ordered.load_order.size() == 2, "both mods included");
    require(ordered.load_order[0] == "base.lib", "dependency loads before dependent");

    const auto missing = dmod::resolve_load_order({addon}, {0, 1, 0});
    require(!missing.success(), "missing required dependency rejected");

    auto cycle_a = make_manifest("cycle.a");
    auto cycle_b = make_manifest("cycle.b");
    cycle_a.dependencies.push_back({"cycle.b", {{1, 0, 0}, std::nullopt}, false});
    cycle_b.dependencies.push_back({"cycle.a", {{1, 0, 0}, std::nullopt}, false});
    const auto cycle = dmod::resolve_load_order({cycle_a, cycle_b}, {0, 1, 0});
    require(!cycle.success(), "dependency cycle rejected");

    std::cout << "Dream Engine DMod manifest tests passed.\\n";
    return EXIT_SUCCESS;
}
