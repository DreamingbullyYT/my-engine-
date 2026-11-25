#include <dream/core/version.hpp>

#include <string>

namespace dream::core {

std::string version_string() {
    constexpr Version version = engine_version();
    return std::to_string(version.major) + "." +
           std::to_string(version.minor) + "." +
           std::to_string(version.patch);
}

} // namespace dream::core
