#include <dream/assets/dependency_graph.hpp>

#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace {
void require(bool condition, std::string_view message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}
}

int main() {
    using namespace dream::assets;

    const std::vector<AssetDefinition> graph{
        {"scene/main", {{"mesh/player", false}, {"texture/player", false}}},
        {"texture/player", {}},
        {"mesh/player", {{"material/player", false}}},
        {"material/player", {{"shader/basic", false}}},
        {"shader/basic", {}}
    };
    const auto plan = resolve_dependencies(graph);
    require(plan.resolved(), "valid graph resolves");
    require(plan.load_order == std::vector<std::string>{
        "shader/basic", "material/player", "mesh/player", "texture/player", "scene/main"
    }, "dependencies precede dependents with deterministic lexical tie-breaking");

    const auto repeated = resolve_dependencies(graph);
    require(repeated.load_order == plan.load_order, "ordering is deterministic");

    const std::vector<AssetDefinition> missing{
        {"scene", {{"absent/required", false}, {"absent/optional", true}}}
    };
    const auto missing_plan = resolve_dependencies(missing);
    require(missing_plan.status == DependencyPlanStatus::MissingDependency,
            "missing required dependency fails planning");
    require(missing_plan.missing_required == std::vector<std::string>{"absent/required"},
            "required missing dependency is reported");
    require(missing_plan.missing_optional == std::vector<std::string>{"absent/optional"},
            "optional missing dependency is reported");

    const std::vector<AssetDefinition> optional_only{
        {"scene", {{"optional/feature", true}}}
    };
    const auto optional_plan = resolve_dependencies(optional_only);
    require(optional_plan.resolved(), "missing optional dependency does not block loading");
    require(optional_plan.missing_optional.size() == 1,
            "missing optional dependency remains visible to caller");

    const std::vector<AssetDefinition> cycle{
        {"a", {{"b", false}}}, {"b", {{"c", false}}}, {"c", {{"a", false}}}
    };
    const auto cycle_plan = resolve_dependencies(cycle);
    require(cycle_plan.status == DependencyPlanStatus::CycleDetected,
            "dependency cycle is rejected");
    require(cycle_plan.load_order.empty(), "partial order is not exposed on cycle");
    require(cycle_plan.blocked_assets.size() == 3, "blocked cycle members are reported");

    const std::vector<AssetDefinition> duplicate_assets{{"same", {}}, {"same", {}}};
    require(resolve_dependencies(duplicate_assets).status == DependencyPlanStatus::DuplicateAsset,
            "duplicate asset keys are rejected");

    const std::vector<AssetDefinition> duplicate_dependencies{
        {"asset", {{"base", true}, {"base", false}}}, {"base", {}}
    };
    require(resolve_dependencies(duplicate_dependencies).status ==
                DependencyPlanStatus::DuplicateDependency,
            "duplicate dependency declarations are rejected");

    const std::vector<AssetDefinition> invalid_key{{"", {}}};
    require(resolve_dependencies(invalid_key).status == DependencyPlanStatus::InvalidKey,
            "empty asset key is rejected");

    const std::vector<AssetDefinition> empty{};
    require(resolve_dependencies(empty).resolved(), "empty graph is valid");

    std::cout << "Dream Engine asset dependency tests passed.\n";
    return EXIT_SUCCESS;
}
