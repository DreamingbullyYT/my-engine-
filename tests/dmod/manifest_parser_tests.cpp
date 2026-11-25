#include <dream/dmod/manifest_parser.hpp>

#include <cstdlib>
#include <iostream>
#include <string_view>

namespace {

void require(bool condition, std::string_view message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << std::endl;
        std::exit(EXIT_FAILURE);
    }
}

} // namespace

int main() {
    constexpr std::string_view valid = R"toml(
format_version = 1
id = "sample.core"
name = "Sample Core"
version = "1.2.3"
engine_api_min = "0.1.0"
engine_api_max = "1.0.0"
entry_script = "scripts/main.dms"
capabilities = ["world.read", "events.subscribe"]

[[dependencies]]
id = "base.library"
min_version = "2.0.0"
max_version = "3.0.0"
optional = false
)toml";

    const auto parsed = dream::dmod::parse_manifest(valid);
    require(parsed.valid(), "valid TOML manifest parses");
    require(parsed.manifest->id == "sample.core", "manifest ID parsed");
    require(parsed.manifest->version.major == 1 &&
            parsed.manifest->version.minor == 2 &&
            parsed.manifest->version.patch == 3, "semantic version parsed");
    require(parsed.manifest->dependencies.size() == 1, "dependency parsed");
    require(parsed.manifest->capabilities.size() == 2, "capabilities parsed");

    const auto bad_version = dream::dmod::parse_manifest(R"toml(
format_version = 1
id = "sample.core"
name = "Sample Core"
version = "1.2"
engine_api_min = "0.1.0"
entry_script = "main.dms"
)toml");
    require(!bad_version.valid(), "malformed semantic version rejected");

    const auto bad_toml = dream::dmod::parse_manifest("format_version = [");
    require(!bad_toml.valid(), "malformed TOML rejected");

    const auto too_large = dream::dmod::parse_manifest(std::string(64 * 1024 + 1, ' '));
    require(!too_large.valid(), "oversized manifest rejected");

    std::cout << "Dream Engine DMod manifest parser tests passed." << std::endl;
    return EXIT_SUCCESS;
}
