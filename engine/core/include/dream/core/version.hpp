#pragma once

#include <cstdint>
#include <string_view>

namespace dream::core {

struct Version final {
    std::uint32_t major;
    std::uint32_t minor;
    std::uint32_t patch;

    [[nodiscard]] constexpr bool operator==(const Version&) const noexcept = default;
};

[[nodiscard]] constexpr Version engine_version() noexcept {
    return {0, 1, 0};
}

[[nodiscard]] constexpr std::string_view engine_name() noexcept {
    return "Dream Engine";
}

[[nodiscard]] std::string_view version_string() noexcept;

} // namespace dream::core
