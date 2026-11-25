#pragma once

#include <dream/core/version.hpp>

#include <optional>
#include <string>
#include <vector>

namespace dream::dmod {

struct VersionRange final {
    core::Version minimum_inclusive{};
    std::optional<core::Version> maximum_exclusive;
};

struct Dependency final {
    std::string id;
    VersionRange versions;
    bool optional{false};
};

struct Manifest final {
    std::string id;
    std::string name;
    core::Version version{};
    VersionRange engine_api;
    std::string entry_script;
    std::vector<Dependency> dependencies;
    std::vector<std::string> capabilities;
};

struct ManifestValidation final {
    std::vector<std::string> errors;

    [[nodiscard]] bool valid() const noexcept { return errors.empty(); }
};

[[nodiscard]] ManifestValidation validate_manifest(const Manifest& manifest);

} // namespace dream::dmod
