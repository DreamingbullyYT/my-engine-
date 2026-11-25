#include <dream/dmod/resolver.hpp>

#include <functional>
#include <map>
#include <queue>
#include <set>

namespace dream::dmod {
namespace {

bool less_than(const core::Version& left, const core::Version& right) {
    if (left.major != right.major) return left.major < right.major;
    if (left.minor != right.minor) return left.minor < right.minor;
    return left.patch < right.patch;
}

bool in_range(const core::Version& version, const VersionRange& range) {
    return !less_than(version, range.minimum_inclusive) &&
        (!range.maximum_exclusive || less_than(version, *range.maximum_exclusive));
}

} // namespace

ResolutionResult resolve_load_order(
    const std::vector<Manifest>& manifests,
    core::Version engine_api_version) {
    ResolutionResult result;
    std::map<std::string, const Manifest*> by_id;

    for (const auto& manifest : manifests) {
        const auto validation = validate_manifest(manifest);
        if (!validation.valid()) {
            for (const auto& error : validation.errors) {
                result.errors.emplace_back(manifest.id + ": " + error);
            }
            continue;
        }
        if (!by_id.emplace(manifest.id, &manifest).second) {
            result.errors.emplace_back("Duplicate installed mod ID: " + manifest.id);
        }
        if (!in_range(engine_api_version, manifest.engine_api)) {
            result.errors.emplace_back("Engine API version is incompatible with mod: " + manifest.id);
        }
    }

    if (!result.errors.empty()) return result;

    std::map<std::string, std::set<std::string>> dependents;
    std::map<std::string, std::size_t> indegree;
    for (const auto& [id, manifest] : by_id) {
        indegree.emplace(id, 0);
        for (const auto& dependency : manifest->dependencies) {
            const auto found = by_id.find(dependency.id);
            if (found == by_id.end()) {
                if (!dependency.optional) {
                    result.errors.emplace_back("Missing required dependency " +
                        dependency.id + " for " + id);
                }
                continue;
            }
            if (!in_range(found->second->version, dependency.versions)) {
                if (!dependency.optional) {
                    result.errors.emplace_back("Dependency version is incompatible: " +
                        dependency.id + " required by " + id);
                }
                continue;
            }
            dependents[dependency.id].insert(id);
            ++indegree[id];
        }
    }

    if (!result.errors.empty()) return result;

    std::priority_queue<std::string, std::vector<std::string>, std::greater<>> ready;
    for (const auto& [id, count] : indegree) {
        if (count == 0) ready.push(id);
    }

    while (!ready.empty()) {
        auto id = ready.top();
        ready.pop();
        result.load_order.push_back(id);

        const auto found = dependents.find(id);
        if (found == dependents.end()) continue;
        for (const auto& dependent : found->second) {
            auto& count = indegree[dependent];
            if (--count == 0) ready.push(dependent);
        }
    }

    if (result.load_order.size() != by_id.size()) {
        result.load_order.clear();
        result.errors.emplace_back("Dependency cycle detected among installed mods.");
    }

    return result;
}

} // namespace dream::dmod
