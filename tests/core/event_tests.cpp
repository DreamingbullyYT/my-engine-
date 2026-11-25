#include <dream/core/event.hpp>

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
struct NumberEvent { int value; };
}

int main() {
    using namespace dream::core;
    EventDispatcher events;
    std::vector<std::string> order;

    auto low = events.subscribe<NumberEvent>(
        [&](const NumberEvent&) { order.emplace_back("low"); }, -1);
    auto high = events.subscribe<NumberEvent>(
        [&](const NumberEvent&) { order.emplace_back("high"); }, 10);
    auto middle = events.subscribe<NumberEvent>(
        [&](const NumberEvent&) { order.emplace_back("middle"); }, 0);
    auto failing = events.subscribe<NumberEvent>(
        [](const NumberEvent&) { throw std::runtime_error("listener failure"); }, 5);

    const auto report = events.dispatch(NumberEvent{42});
    require(report.invoked == 3, "all non-failing active listeners invoked");
    require(report.failed == 1, "listener exception isolated and counted");
    require(order == std::vector<std::string>{"high", "middle", "low"},
            "listeners execute by descending priority");

    middle.disconnect();
    order.clear();
    const auto second = events.dispatch(NumberEvent{7});
    require(second.invoked == 2 && second.failed == 1,
            "disconnected listener is not invoked");
    require(order == std::vector<std::string>{"high", "low"},
            "remaining listeners preserve priority order");

    std::cout << "Dream Engine event tests passed.\n";
    return EXIT_SUCCESS;
}
