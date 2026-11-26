#pragma once

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <functional>
#include <limits>
#include <type_traits>
#include <utility>

namespace dream::core {

struct FixedStepConfig final {
    double fixed_delta_seconds{1.0 / 60.0};
    double max_frame_delta_seconds{0.25};
    std::uint32_t max_steps_per_frame{8};
};

enum class FixedStepError : std::uint8_t {
    None,
    InvalidConfiguration,
    InvalidDelta,
    AlreadyUpdating,
    AccumulatorOverflow,
    TickOverflow
};

struct FixedStepAdvanceResult final {
    FixedStepError error{FixedStepError::None};
    std::uint32_t steps_executed{0};
    std::uint64_t tick_index{0};
    double interpolation_alpha{0.0};
    double dropped_seconds{0.0};
    double total_dropped_seconds{0.0};
    bool frame_delta_clamped{false};

    [[nodiscard]] explicit operator bool() const noexcept {
        return error == FixedStepError::None;
    }
};

// Single-owner fixed-step accumulator for simulation. It is separate from the
// render loop: call advance() once per rendered frame, then interpolate using
// interpolation_alpha. Step callbacks receive (fixed_dt, next_tick_index).
//
// Large frame deltas are clamped, and whole-step backlog beyond
// max_steps_per_frame is dropped while preserving the fractional remainder.
// This avoids a spiral of death at the cost of simulation time under overload.
// The class is not generally thread-safe; concurrent/reentrant advance() calls
// are rejected. configure() and reset() must not race with advance().
//
// If a step callback throws, the exception propagates, the failed step remains
// in the accumulator and its tick index is not committed. The callback should
// be exception-safe because it may have partially modified simulation state.
class FixedStepClock final {
public:
    explicit FixedStepClock(FixedStepConfig config = {}) noexcept
        : config_(config), valid_(valid_config(config)) {}

    FixedStepClock(const FixedStepClock&) = delete;
    FixedStepClock& operator=(const FixedStepClock&) = delete;

    [[nodiscard]] bool configure(FixedStepConfig config) noexcept {
        if (!valid_config(config)) return false;
        config_ = config;
        reset();
        valid_ = true;
        return true;
    }

    [[nodiscard]] bool valid() const noexcept { return valid_; }

    [[nodiscard]] const FixedStepConfig& config() const noexcept {
        return config_;
    }

    void reset() noexcept {
        accumulator_seconds_ = 0.0;
        total_dropped_seconds_ = 0.0;
        tick_index_ = 0;
    }

    [[nodiscard]] double accumulated_seconds() const noexcept {
        return accumulator_seconds_;
    }

    [[nodiscard]] double total_dropped_seconds() const noexcept {
        return total_dropped_seconds_;
    }

    [[nodiscard]] std::uint64_t tick_index() const noexcept {
        return tick_index_;
    }

    template <typename StepCallback>
    [[nodiscard]] FixedStepAdvanceResult advance(
        double frame_delta_seconds, StepCallback&& step_callback) {
        static_assert(std::is_invocable_v<
                          std::decay_t<StepCallback>, double, std::uint64_t>,
                      "Fixed-step callback must accept (double fixed_dt, uint64_t tick)");

        FixedStepAdvanceResult result;
        result.tick_index = tick_index_;
        result.total_dropped_seconds = total_dropped_seconds_;

        if (!valid_) {
            result.error = FixedStepError::InvalidConfiguration;
            return result;
        }
        if (!std::isfinite(frame_delta_seconds) || frame_delta_seconds < 0.0) {
            result.error = FixedStepError::InvalidDelta;
            return result;
        }

        if (updating_.test_and_set(std::memory_order_acquire)) {
            result.error = FixedStepError::AlreadyUpdating;
            return result;
        }
        struct UpdateGuard final {
            std::atomic_flag& flag;
            ~UpdateGuard() { flag.clear(std::memory_order_release); }
        } guard{updating_};

        const double accepted_delta = std::min(
            frame_delta_seconds, config_.max_frame_delta_seconds);
        result.frame_delta_clamped = accepted_delta < frame_delta_seconds;
        result.dropped_seconds = frame_delta_seconds - accepted_delta;

        if (!std::isfinite(accumulator_seconds_ + accepted_delta)) {
            result.error = FixedStepError::AccumulatorOverflow;
            result.dropped_seconds = 0.0;
            return result;
        }
        accumulator_seconds_ += accepted_delta;

        const double step_tolerance =
            std::numeric_limits<double>::epsilon() *
            std::max({1.0, std::abs(accumulator_seconds_),
                      std::abs(config_.fixed_delta_seconds)}) * 8.0;

        while (accumulator_seconds_ + step_tolerance >=
                   config_.fixed_delta_seconds &&
               result.steps_executed < config_.max_steps_per_frame) {
            if (tick_index_ == std::numeric_limits<std::uint64_t>::max()) {
                result.error = FixedStepError::TickOverflow;
                break;
            }

            const std::uint64_t next_tick = tick_index_ + 1U;
            std::invoke(step_callback, config_.fixed_delta_seconds, next_tick);

            accumulator_seconds_ -= config_.fixed_delta_seconds;
            if (std::abs(accumulator_seconds_) <= step_tolerance)
                accumulator_seconds_ = 0.0;
            tick_index_ = next_tick;
            ++result.steps_executed;
        }

        if (result.error == FixedStepError::None &&
            accumulator_seconds_ + step_tolerance >=
                config_.fixed_delta_seconds) {
            const double remainder = std::fmod(
                accumulator_seconds_, config_.fixed_delta_seconds);
            result.dropped_seconds += accumulator_seconds_ - remainder;
            accumulator_seconds_ = remainder;
        }

        if (result.dropped_seconds > 0.0) {
            total_dropped_seconds_ = saturating_add(
                total_dropped_seconds_, result.dropped_seconds);
        }

        result.tick_index = tick_index_;
        result.interpolation_alpha = std::clamp(
            accumulator_seconds_ / config_.fixed_delta_seconds, 0.0, 1.0);
        result.total_dropped_seconds = total_dropped_seconds_;
        return result;
    }

private:
    [[nodiscard]] static bool valid_config(FixedStepConfig config) noexcept {
        return std::isfinite(config.fixed_delta_seconds) &&
               std::isfinite(config.max_frame_delta_seconds) &&
               config.fixed_delta_seconds > 0.0 &&
               config.max_frame_delta_seconds >= config.fixed_delta_seconds &&
               config.max_steps_per_frame > 0U;
    }

    [[nodiscard]] static double saturating_add(double left, double right) noexcept {
        if (right > std::numeric_limits<double>::max() - left)
            return std::numeric_limits<double>::max();
        return left + right;
    }

    FixedStepConfig config_{};
    bool valid_{true};
    double accumulator_seconds_{0.0};
    double total_dropped_seconds_{0.0};
    std::uint64_t tick_index_{0};
    std::atomic_flag updating_ = ATOMIC_FLAG_INIT;
};

} // namespace dream::core
