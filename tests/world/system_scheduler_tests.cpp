#include <dream/world/scene.hpp>

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}
}

int main() {
    using namespace dream::world;

    Scene scene("Main");
    require(scene.name() == "Main", "scene retains its name");
    const auto entity = scene.world().create_entity();
    require(entity, "scene owns an independent entity world");

    std::vector<std::string> order;
    auto pre_high = scene.systems().add_system(
        "pre-high", SystemPhase::PreUpdate, 10,
        [&order](World&, const SystemContext& context) {
            require(context.delta_seconds == 0.016F, "system receives frame delta");
            require(context.frame_index == 1, "system receives one-based frame index");
            order.emplace_back("pre-high");
        });
    auto pre_low = scene.systems().add_system(
        "pre-low", SystemPhase::PreUpdate, -1,
        [&order](World&, const SystemContext&) { order.emplace_back("pre-low"); });
    auto update = scene.systems().add_system(
        "update", SystemPhase::Update, 0,
        [&order](World&, const SystemContext&) { order.emplace_back("update"); });
    auto post = scene.systems().add_system(
        "post", SystemPhase::PostUpdate, 0,
        [&order](World&, const SystemContext&) { order.emplace_back("post"); });
    auto failure = scene.systems().add_system(
        "failure", SystemPhase::Update, 5,
        [](World&, const SystemContext&) { throw std::runtime_error("system failed"); });

    require(pre_high && pre_low && update && post && failure,
            "valid systems register successfully");
    const auto first = scene.update(0.016F);
    require(first.error == SystemUpdateError::None, "valid delta updates the scene");
    require(first.executed == 5 && first.failures.size() == 1,
            "system failure is reported without aborting remaining systems");
    require(first.failures[0].name == "failure" &&
            first.failures[0].message == "system failed",
            "system failure includes name and message");
    require(order == std::vector<std::string>{
                "pre-high", "pre-low", "update", "post"},
            "systems run by phase, descending priority and stable registration order");

    const auto bad_delta = scene.update(-0.1F);
    require(bad_delta.error == SystemUpdateError::InvalidDeltaTime,
            "negative delta is rejected");
    require(scene.systems().frame_index() == 1,
            "invalid delta does not advance frame index");

    SystemScheduler dynamic;
    World dynamic_world;
    std::vector<std::string> dynamic_order;
    SystemHandle target_handle{};
    SystemHandle late_handle{};
    bool late_added = false;

    const auto target = dynamic.add_system(
        "target", SystemPhase::Update, 0,
        [&dynamic_order](World&, const SystemContext&) {
            dynamic_order.emplace_back("target");
        });
    target_handle = target.handle;
    auto remover = dynamic.add_system(
        "remover", SystemPhase::Update, 10,
        [&dynamic, &dynamic_order, &target_handle, &late_handle, &late_added]
        (World&, const SystemContext&) {
            dynamic_order.emplace_back("remover");
            if (target_handle) {
                require(dynamic.remove_system(target_handle),
                        "system can remove a later system during a tick");
                target_handle = {};
            }
            if (!late_added) {
                const auto added = dynamic.add_system(
                    "late", SystemPhase::PostUpdate, 0,
                    [&dynamic_order](World&, const SystemContext&) {
                        dynamic_order.emplace_back("late");
                    });
                require(static_cast<bool>(added), "system can register another system during a tick");
                late_handle = added.handle;
                late_added = true;
            }
        });
    require(target && remover, "dynamic systems register");

    const auto dynamic_first = dynamic.update(dynamic_world, 0.01F);
    require(dynamic_first.executed == 1 && dynamic_first.skipped == 1,
            "removed system is skipped from the current snapshot");
    require(dynamic_order == std::vector<std::string>{"remover"},
            "new system does not run until the next tick");

    require(dynamic.remove_system(remover.handle), "system can be removed between ticks");
    const auto dynamic_second = dynamic.update(dynamic_world, 0.01F);
    require(dynamic_second.executed == 1 &&
            dynamic_order == std::vector<std::string>{"remover", "late"},
            "newly registered system runs on the next tick");
    require(dynamic.remove_system(late_handle), "late system can be removed");
    require(!dynamic.remove_system(late_handle), "stale system handle is rejected");

    SystemScheduler reentrant;
    auto recursive = reentrant.add_system(
        "recursive", SystemPhase::Update, 0,
        [&reentrant](World& nested_world, const SystemContext&) {
            const auto nested = reentrant.update(nested_world, 0.0F);
            require(nested.error == SystemUpdateError::AlreadyUpdating,
                    "recursive update is rejected without deadlock");
        });
    require(recursive, "recursive guard system registers");
    const auto guarded = reentrant.update(dynamic_world, 0.0F);
    require(guarded.error == SystemUpdateError::None && guarded.executed == 1,
            "outer update completes");

    const auto empty_name = dynamic.add_system(
        "", SystemPhase::Update, 0, [](World&, const SystemContext&) {});
    require(empty_name.error == SystemAddError::InvalidName,
            "empty system name is rejected");
    const auto no_callback = dynamic.add_system(
        "empty", SystemPhase::Update, 0, {});
    require(no_callback.error == SystemAddError::EmptyCallback,
            "empty callback is rejected");

    std::cout << "Dream Engine system scheduler tests passed.\n";
    return EXIT_SUCCESS;
}
