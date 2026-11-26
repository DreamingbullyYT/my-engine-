#include <dream/assets/asset_request_coordinator.hpp>

#include <atomic>
#include <cstdlib>
#include <future>
#include <iostream>
#include <stdexcept>
#include <string_view>

namespace {
void require(bool condition, std::string_view message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}
}

int main() {
    using Coordinator = dream::assets::AssetRequestCoordinator<int>;
    Coordinator coordinator;
    std::atomic<int> factory_calls{0};
    std::promise<void> entered;
    std::promise<void> release;
    auto release_signal = release.get_future().share();

    auto first = coordinator.request("texture/player", [&] {
        ++factory_calls;
        return std::async(std::launch::async, [&] {
            entered.set_value();
            release_signal.wait();
            return 42;
        });
    });

    entered.get_future().wait();
    auto second = coordinator.request("texture/player", [&] {
        ++factory_calls;
        return std::async(std::launch::async, [] { return 99; });
    });

    require(factory_calls.load() == 1,
            "concurrent requests for the same key invoke only one factory");
    require(coordinator.in_flight_count() == 1,
            "pending request remains registered");
    release.set_value();

    require(first.get() == 42 && second.get() == 42,
            "all callers observe the same shared result");
    coordinator.prune_completed();
    require(coordinator.in_flight_count() == 0,
            "completed request is removed from the in-flight registry");

    auto next = coordinator.request("texture/player", [&] {
        ++factory_calls;
        return std::async(std::launch::async, [] { return 7; });
    });
    require(next.get() == 7,
            "a later request can start after the previous request completes");
    require(factory_calls.load() == 2,
            "completed requests are not retained as a result cache");

    bool rejected_empty_key = false;
    try {
        (void)coordinator.request("", [] {
            return std::async(std::launch::async, [] { return 1; });
        });
    } catch (const std::invalid_argument&) {
        rejected_empty_key = true;
    }
    require(rejected_empty_key, "empty request keys are rejected");

    std::cout << "Dream Engine asset request coordinator tests passed.\\n";
    return EXIT_SUCCESS;
}
