#include <dream/core/version.hpp>

#include <cstdlib>
#include <iostream>
#include <string_view>

namespace {

void require(bool condition, std::string_view message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

} // namespace

int main() {
    using dream::core::engine_name;
    using dream::core::engine_version;
    using dream::core::version_string;

    constexpr auto version = engine_version();
    static_assert(version.major == 0);
    static_assert(version.minor == 1);
    static_assert(version.patch == 0);

    require(engine_name() == "Dream Engine", "engine name");
    require(version_string() == "0.1.0", "version string");

    std::cout << "Dream Engine core tests passed.\n";
    return EXIT_SUCCESS;
}
