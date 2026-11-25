#pragma once

#include <dream/dmod/manifest.hpp>

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace dream::dmod {

struct ManifestParseResult final {
    std::optional<Manifest> manifest;
    std::vector<std::string> errors;

    [[nodiscard]] bool valid() const noexcept {
        return manifest.has_value() && errors.empty();
    }
};

[[nodiscard]] ManifestParseResult parse_manifest(std::string_view source);

} // namespace dream::dmod
