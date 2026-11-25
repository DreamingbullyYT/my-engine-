#include <dream/platform/application.hpp>

#include <algorithm>
#include <cmath>
#include <exception>
#include <utility>

namespace dream::platform {

Application::Application(
    IPlatformBackend& backend,
    ApplicationConfig config,
    ApplicationCallbacks callbacks)
    : backend_(backend),
      config_(std::move(config)),
      callbacks_(std::move(callbacks)) {}

bool Application::run() {
    if (state_.load(std::memory_order_acquire) != ApplicationState::Created) {
        error_ = "Application::run() may only be called once.";
        return false;
    }

    if (config_.name.empty() ||
        !std::isfinite(config_.max_frame_delta_seconds) ||
        config_.max_frame_delta_seconds <= 0.0) {
        error_ = "Application configuration is invalid.";
        state_.store(ApplicationState::Failed, std::memory_order_release);
        return false;
    }

    state_.store(ApplicationState::Initializing, std::memory_order_release);

    bool backend_started = false;
    bool application_initialization_entered = false;
    bool cleanup_failed = false;

    const auto cleanup = [&]() noexcept {
        if (application_initialization_entered) {
            application_initialization_entered = false;
            if (callbacks_.on_shutdown) {
                try {
                    callbacks_.on_shutdown();
                } catch (const std::exception& exception) {
                    if (error_.empty()) {
                        error_ = std::string("Shutdown callback failed: ") + exception.what();
                    }
                    cleanup_failed = true;
                } catch (...) {
                    if (error_.empty()) error_ = "Shutdown callback failed with an unknown exception.";
                    cleanup_failed = true;
                }
            }
        }

        if (backend_started) {
            backend_started = false;
            backend_.shutdown();
        }
    };

    try {
        std::string backend_error;
        backend_started = true;
        if (!backend_.initialize(config_, backend_error)) {
            error_ = backend_error.empty()
                ? "Platform backend initialization failed."
                : std::move(backend_error);
            cleanup();
            state_.store(ApplicationState::Failed, std::memory_order_release);
            return false;
        }

        application_initialization_entered = true;
        if (callbacks_.on_initialize && !callbacks_.on_initialize()) {
            error_ = "Application initialization callback rejected startup.";
            cleanup();
            state_.store(ApplicationState::Failed, std::memory_order_release);
            return false;
        }

        state_.store(ApplicationState::Running, std::memory_order_release);
        double previous_time = backend_.monotonic_time_seconds();

        while (!stop_requested_.load(std::memory_order_acquire)) {
            if (!backend_.poll_events()) break;

            const double current_time = backend_.monotonic_time_seconds();
            double delta_seconds = 0.0;
            if (std::isfinite(current_time) && std::isfinite(previous_time) &&
                current_time >= previous_time) {
                delta_seconds = std::min(
                    current_time - previous_time,
                    config_.max_frame_delta_seconds);
            }
            if (std::isfinite(current_time)) previous_time = current_time;

            if (callbacks_.on_update) callbacks_.on_update(delta_seconds);
        }

        cleanup();
        if (cleanup_failed) {
            state_.store(ApplicationState::Failed, std::memory_order_release);
            return false;
        }

        state_.store(ApplicationState::Stopped, std::memory_order_release);
        return true;
    } catch (const std::exception& exception) {
        if (error_.empty()) error_ = std::string("Application failed: ") + exception.what();
    } catch (...) {
        if (error_.empty()) error_ = "Application failed with an unknown exception.";
    }

    cleanup();
    state_.store(ApplicationState::Failed, std::memory_order_release);
    return false;
}

void Application::request_stop() noexcept {
    stop_requested_.store(true, std::memory_order_release);
}

ApplicationState Application::state() const noexcept {
    return state_.load(std::memory_order_acquire);
}

const std::string& Application::error() const noexcept {
    return error_;
}

} // namespace dream::platform
