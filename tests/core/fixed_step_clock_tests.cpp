#include <dream/core/fixed_step_clock.hpp>

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace {
using dream::core::FixedStepClock;
using dream::core::FixedStepConfig;
using dream::core::FixedStepError;

void require(bool condition, std::string_view message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

bool near(double left, double right, double epsilon = 1.0e-9) {
    return std::abs(left - right) <= epsilon;
}
}

int main() {
    FixedStepClock clock(FixedStepConfig{0.1, 0.5, 8});
    require(clock.valid(), "valid clock configuration is accepted");

    std::vector<std::uint64_t> ticks;
    const auto first = clock.advance(
        0.25, [&ticks](double delta, std::uint64_t tick) {
            require(near(delta, 0.1), "step callback receives fixed delta");
            ticks.push_back(tick);
        });
    require(first && first.steps_executed == 2U && first.tick_index == 2U,
            "fractional frame time produces the expected fixed steps");
    require(ticks == std::vector<std::uint64_t>{1U, 2U},
            "simulation tick indices are monotonic and one-based");
    require(near(first.interpolation_alpha, 0.5),
            "fractional remainder becomes interpolation alpha");
    require(near(clock.accumulated_seconds(), 0.05),
            "fractional time is retained for interpolation");

    const auto zero_delta = clock.advance(
        0.0, [](double, std::uint64_t) {});
    require(zero_delta && zero_delta.steps_executed == 0U &&
            zero_delta.tick_index == 2U,
            "zero frame delta does not advance simulation");

    const auto second = clock.advance(
        0.05, [](double, std::uint64_t) {});
    require(second && second.steps_executed == 1U && second.tick_index == 3U,
            "accumulated fractional time advances the next tick");

    const auto before_invalid = clock.accumulated_seconds();
    const auto negative = clock.advance(-0.1, [](double, std::uint64_t) {});
    require(negative.error == FixedStepError::InvalidDelta,
            "negative frame delta is rejected");
    const auto not_a_number = clock.advance(
        std::numeric_limits<double>::quiet_NaN(), [](double, std::uint64_t) {});
    require(not_a_number.error == FixedStepError::InvalidDelta,
            "NaN frame delta is rejected");
    require(clock.accumulated_seconds() == before_invalid &&
            clock.tick_index() == 3U,
            "invalid frame deltas do not mutate clock state");

    FixedStepClock overloaded(FixedStepConfig{0.1, 1.0, 2});
    const auto overload_result = overloaded.advance(
        0.55, [](double, std::uint64_t) {});
    require(overload_result && overload_result.steps_executed == 2U,
            "per-frame step cap is respected");
    require(near(overload_result.dropped_seconds, 0.3, 1.0e-8),
            "whole-step backlog beyond the cap is dropped");
    require(near(overload_result.interpolation_alpha, 0.5, 1.0e-8),
            "overload drop preserves fractional remainder");

    FixedStepClock clamped(FixedStepConfig{0.1, 0.3, 10});
    const auto clamped_result = clamped.advance(
        0.6, [](double, std::uint64_t) {});
    require(clamped_result && clamped_result.frame_delta_clamped,
            "long frame delta is clamped");
    require(clamped_result.steps_executed == 3U,
            "clamped frame executes only its accepted fixed steps");
    require(near(clamped_result.dropped_seconds, 0.3, 1.0e-8),
            "time removed by frame clamp is reported");
    require(near(clamped.total_dropped_seconds(), 0.3, 1.0e-8),
            "dropped time is accumulated");

    FixedStepClock invalid(FixedStepConfig{0.0, 1.0, 4});
    require(!invalid.valid(), "invalid constructor configuration is exposed");
    require(invalid.advance(0.1, [](double, std::uint64_t) {}).error ==
                FixedStepError::InvalidConfiguration,
            "invalid clock refuses to advance");
    require(!invalid.configure(FixedStepConfig{0.1, 0.05, 2}),
            "max frame delta smaller than fixed delta is rejected");
    require(invalid.configure(FixedStepConfig{0.02, 0.2, 4}),
            "valid configuration can be installed");
    require(invalid.valid(), "valid reconfiguration enables the clock");

    FixedStepClock reentrant(FixedStepConfig{0.1, 0.2, 2});
    FixedStepError nested_error = FixedStepError::None;
    const auto reentrant_result = reentrant.advance(
        0.1, [&reentrant, &nested_error](double, std::uint64_t) {
            const auto nested = reentrant.advance(
                0.1, [](double, std::uint64_t) {});
            nested_error = nested.error;
        });
    require(reentrant_result && reentrant_result.steps_executed == 1U,
            "outer reentrant-guarded update succeeds");
    require(nested_error == FixedStepError::AlreadyUpdating,
            "nested advance is rejected");

    FixedStepClock throwing(FixedStepConfig{0.1, 0.2, 2});
    bool exception_seen = false;
    try {
        (void)throwing.advance(0.1, [](double, std::uint64_t) {
            throw std::runtime_error("simulation step failed");
        });
    } catch (const std::runtime_error&) {
        exception_seen = true;
    }
    require(exception_seen && throwing.tick_index() == 0U &&
            near(throwing.accumulated_seconds(), 0.1),
            "failed callback does not commit its tick or consume its time");
    const auto retry = throwing.advance(0.0, [](double, std::uint64_t) {});
    require(retry && retry.steps_executed == 1U && retry.tick_index == 1U,
            "uncommitted step may be retried on the next advance");

    clock.reset();
    require(clock.tick_index() == 0U &&
            clock.accumulated_seconds() == 0.0 &&
            clock.total_dropped_seconds() == 0.0,
            "reset clears accumulated time and tick counters");

    std::cout << "Dream Engine fixed-step clock tests passed.\n";
    return EXIT_SUCCESS;
}
