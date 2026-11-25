#include <dream/dmod/manifest.hpp>

#include <algorithm>
#include <cctype>
#include <unordered_set>

namespace dream::dmod {
namespace {

bool valid_id(const std::string& id) {
    if (id.empty() || id.size() > 128) {
        return false;
    }

    bool segment_start = true;
    for (const unsigned char character : id) {
        if (character == '.') {
            if (segment_start) {
                return false;
            }
            segment_start = true;
            continue;
        }

        if (!(std::islower(character) || std::isdigit(character) ||
              character == '_' || character == '-')) {
            return false;
        }
        segment_start = false;
    }
    return !segment_start;
}

bool valid_relative_script_path(const std::string& path) {
    if (path.empty() || path.size() > 512 || path.front() == '/' || path.back() == '/' ||
        path.find('\\') != std::string::npos || path.find(':') != std::string::npos) {
        return false;
    }

    std::size_t begin = 0;
    while (begin < path.size()) {
        const auto end = path.find('/', begin);
        const auto segment = path.substr(begin, end == std::string::npos
            ? std::string::npos : end - begin);
        if (segment.empty() || segment == "." || segment == "..") {
            return false;
        }
        begin = end == std::string::npos ? path.size() : end + 1;
    }
    return true;
}

bool less_than(const core::Version& left, const core::Version& right) {
    if (left.major != right.major) return left.major < right.major;
    if (left.minor != right.minor) return left.minor < right.minor;
    return left.patch < right.patch;
}

} // namespace

ManifestValidation validate_manifest(const Manifest& manifest) {
    ManifestValidation result;

    if (!valid_id(manifest.id)) {
        result.errors.emplace_back("Manifest ID must be lowercase dot-separated segments using [a-z0-9_-].");
    }
    if (manifest.name.empty() || manifest.name.size() > 256) {
        result.errors.emplace_back("Manifest name must contain between 1 and 256 bytes.");
    }
    if (!valid_relative_script_path(manifest.entry_script)) {
        result.errors.emplace_back("Entry script must be a safe relative path without traversal.");
    }
    if (manifest.engine_api.maximum_exclusive &&
        !less_than(manifest.engine_api.minimum_inclusive,
                   *manifest.engine_api.maximum_exclusive)) {
        result.errors.emplace_back("Engine API version range is empty or reversed.");
    }

    std::unordered_set<std::string> dependency_ids;
    for (const auto& dependency : manifest.dependencies) {
        if (!valid_id(dependency.id)) {
            result.errors.emplace_back("Dependency ID is invalid: " + dependency.id);
        } else if (dependency.id == manifest.id) {
            result.errors.emplace_back("A mod cannot depend on itself.");
        }
        if (!dependency_ids.insert(dependency.id).second) {
            result.errors.emplace_back("Duplicate dependency: " + dependency.id);
        }
        if (dependency.versions.maximum_exclusive &&
            !less_than(dependency.versions.minimum_inclusive,
                       *dependency.versions.maximum_exclusive)) {
            result.errors.emplace_back("Dependency version range is empty or reversed: " + dependency.id);
        }
    }

    std::unordered_set<std::string> capabilities;
    for (const auto& capability : manifest.capabilities) {
        if (capability.empty() || capability.size() > 128 ||
            !std::all_of(capability.begin(), capability.end(), [](unsigned char c) {
                return std::islower(c) || std::isdigit(c) || c == '.' || c == '_' || c == '-';
            })) {
            result.errors.emplace_back("Capability name is invalid: " + capability);
        } else if (!capabilities.insert(capability).second) {
            result.errors.emplace_back("Duplicate capability: " + capability);
        }
    }

    return result;
}

} // namespace dream::dmod
