#pragma once

#include <algorithm>
#include <cstddef>
#include <map>
#include <set>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace dream::assets {

struct AssetDependency final {
    std::string key;
    bool optional{false};
};

struct AssetDefinition final {
    std::string key;
    std::vector<AssetDependency> dependencies;
};

enum class DependencyPlanStatus {
    Resolved,
    InvalidKey,
    DuplicateAsset,
    DuplicateDependency,
    MissingDependency,
    CycleDetected
};

struct DependencyPlan final {
    DependencyPlanStatus status{DependencyPlanStatus::Resolved};
    std::vector<std::string> load_order;
    std::vector<std::string> missing_required;
    std::vector<std::string> missing_optional;
    std::vector<std::string> blocked_assets;

    [[nodiscard]] bool resolved() const noexcept {
        return status == DependencyPlanStatus::Resolved;
    }
};

// Produces a deterministic dependency-first order. This is a planning utility:
// it does not load, own, cache, or unload asset resources.
[[nodiscard]] inline DependencyPlan resolve_dependencies(
    std::span<const AssetDefinition> definitions) {
    DependencyPlan result;
    std::map<std::string, const AssetDefinition*> assets;

    for (const auto& definition : definitions) {
        if (definition.key.empty() || definition.key.size() > 4096) {
            result.status = DependencyPlanStatus::InvalidKey;
            return result;
        }
        if (!assets.emplace(definition.key, &definition).second) {
            result.status = DependencyPlanStatus::DuplicateAsset;
            return result;
        }
    }

    std::map<std::string, std::size_t> indegree;
    std::map<std::string, std::set<std::string>> dependents;
    for (const auto& [key, definition] : assets) {
        indegree.emplace(key, 0);
        std::set<std::string> seen;
        for (const auto& dependency : definition->dependencies) {
            if (dependency.key.empty() || dependency.key.size() > 4096) {
                result.status = DependencyPlanStatus::InvalidKey;
                return result;
            }
            if (!seen.insert(dependency.key).second) {
                result.status = DependencyPlanStatus::DuplicateDependency;
                return result;
            }
            if (!assets.contains(dependency.key)) {
                if (dependency.optional) {
                    result.missing_optional.push_back(dependency.key);
                } else {
                    result.missing_required.push_back(dependency.key);
                }
                continue;
            }
            ++indegree[key];
            dependents[dependency.key].insert(key);
        }
    }

    if (!result.missing_required.empty()) {
        std::sort(result.missing_required.begin(), result.missing_required.end());
        std::sort(result.missing_optional.begin(), result.missing_optional.end());
        result.status = DependencyPlanStatus::MissingDependency;
        return result;
    }

    std::set<std::string> ready;
    for (const auto& [key, count] : indegree) {
        if (count == 0) ready.insert(key);
    }

    while (!ready.empty()) {
        auto current = ready.begin();
        std::string key = *current;
        ready.erase(current);
        result.load_order.push_back(key);

        const auto found = dependents.find(key);
        if (found == dependents.end()) continue;
        for (const auto& dependent : found->second) {
            auto& count = indegree.at(dependent);
            if (--count == 0) ready.insert(dependent);
        }
    }

    if (result.load_order.size() != assets.size()) {
        for (const auto& [key, count] : indegree) {
            if (count != 0) result.blocked_assets.push_back(key);
        }
        result.status = DependencyPlanStatus::CycleDetected;
        result.load_order.clear();
    }
    std::sort(result.missing_optional.begin(), result.missing_optional.end());
    return result;
}

} // namespace dream::assets
