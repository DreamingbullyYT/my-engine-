#pragma once

#include <dream/dmod/manifest.hpp>

#include <string>
#include <vector>

namespace dream::dmod {

struct ResolutionResult final {
    std::vector<std::string> load_order;
    std::vector<std::string> errors;

    [[nodiscard]] bool success() const noexcept { return errors.empty(); }
};

[[nodiscard]] ResolutionResult resolve_load_order(
    const std::vector<Manifest>& manifests,
    core::Version engine_api_version);

} // namespace dream::dmod
