#pragma once

#include <atomic>
#include <cstdint>
#include <functional>
#include <string>

namespace dream::platform {

enum class ApplicationState : std::uint8_t {
    Created,
    Initializing,
    Running,
    Stopped,
    Failed
};

struct ApplicationConfig final {
    std::string name{"Dream Engine"};
    double max_frame_delta_seconds{0.25};
};

class IPlatformBackend {
public:
    virtual ~IPlatformBackend() = default;

    [[nodiscard]] virtual bool initialize(
        const ApplicationConfig& config, std::string& error) = 0;
    [[nodiscard]] virtual bool poll_events() = 0;
    [[nodiscard]] virtual double monotonic_time_seconds() noexcept = 0;
    virtual void shutdown() noexcept = 0;
};

struct ApplicationCallbacks final {
    std::function<bool()> on_initialize;
    std::function<void(double)> on_update;
    std::function<void()> on_shutdown;
};

class Application final {
public:
    explicit Application(
        IPlatformBackend& backend,
        ApplicationConfig config = {},
        ApplicationCallbacks callbacks = {});

    [[nodiscard]] bool run();
    void request_stop() noexcept;

    [[nodiscard]] ApplicationState state() const noexcept;
    [[nodiscard]] const std::string& error() const noexcept;

private:
    IPlatformBackend& backend_;
    ApplicationConfig config_;
    ApplicationCallbacks callbacks_;
    std::atomic<ApplicationState> state_{ApplicationState::Created};
    std::atomic<bool> stop_requested_{false};
    std::string error_;
};

} // namespace dream::platform
