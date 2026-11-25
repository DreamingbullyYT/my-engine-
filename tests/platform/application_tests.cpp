#include <dream/platform/application.hpp>

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string_view>
#include <utility>
#include <vector>

namespace {

void require(bool condition, std::string_view message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << std::endl;
        std::exit(EXIT_FAILURE);
    }
}

class FakeBackend final : public dream::platform::IPlatformBackend {
public:
    bool initialize_result{true};
    bool initialize_called{false};
    bool shutdown_called{false};
    std::string initialize_error;
    std::size_t poll_limit{2};
    std::size_t poll_count{0};
    std::vector<double> times{10.0, 10.1, 10.6};
    std::size_t time_index{0};

    bool initialize(
        const dream::platform::ApplicationConfig&, std::string& error) override {
        initialize_called = true;
        error = initialize_error;
        return initialize_result;
    }

    bool poll_events() override {
        ++poll_count;
        return poll_count <= poll_limit;
    }

    double monotonic_time_seconds() noexcept override {
        if (time_index >= times.size()) return times.empty() ? 0.0 : times.back();
        return times[time_index++];
    }

    void shutdown() noexcept override {
        shutdown_called = true;
    }
};

} // namespace

int main() {
    using namespace dream::platform;

    {
        FakeBackend backend;
        int initialized = 0;
        int updated = 0;
        int shutdown = 0;
        std::vector<double> deltas;

        ApplicationCallbacks callbacks;
        callbacks.on_initialize = [&] {
            ++initialized;
            return true;
        };
        callbacks.on_update = [&](double delta) {
            ++updated;
            deltas.push_back(delta);
        };
        callbacks.on_shutdown = [&] { ++shutdown; };

        Application app(backend, ApplicationConfig{"Test App", 0.2}, callbacks);
        require(app.run(), "application exits normally");
        require(app.state() == ApplicationState::Stopped, "normal stop state");
        require(backend.initialize_called, "backend initialized");
        require(backend.shutdown_called, "backend shut down");
        require(initialized == 1 && shutdown == 1, "lifecycle callbacks run once");
        require(updated == 2, "event pump controls frame count");
        require(deltas.size() == 2, "frame deltas recorded");
        require(std::abs(deltas[0] - 0.1) < 0.000001, "normal frame delta");
        require(std::abs(deltas[1] - 0.2) < 0.000001, "large frame delta is clamped");
        require(!app.run(), "application cannot be run twice");
    }

    {
        FakeBackend backend;
        int shutdown = 0;
        ApplicationCallbacks callbacks;
        callbacks.on_initialize = [] { return false; };
        callbacks.on_shutdown = [&] { ++shutdown; };

        Application app(backend, {}, callbacks);
        require(!app.run(), "failed application initialization is reported");
        require(app.state() == ApplicationState::Failed, "failed init state");
        require(backend.shutdown_called, "backend cleaned after failed app init");
        require(shutdown == 1, "shutdown callback cleans partial app initialization");
    }

    {
        FakeBackend backend;
        backend.initialize_result = false;
        backend.initialize_error = "backend unavailable";
        Application app(backend);
        require(!app.run(), "backend initialization failure is reported");
        require(app.state() == ApplicationState::Failed, "backend failure state");
        require(backend.shutdown_called, "backend shutdown called after failed initialize");
        require(app.error() == "backend unavailable", "backend error is preserved");
    }

    {
        FakeBackend backend;
        int shutdown = 0;
        ApplicationCallbacks callbacks;
        callbacks.on_update = [](double) { throw std::runtime_error("update failure"); };
        callbacks.on_shutdown = [&] { ++shutdown; };

        Application app(backend, {}, callbacks);
        require(!app.run(), "update exception is reported");
        require(app.state() == ApplicationState::Failed, "update exception state");
        require(backend.shutdown_called, "backend cleaned after update exception");
        require(shutdown == 1, "application cleanup runs once after update exception");
    }

    {
        FakeBackend backend;
        ApplicationConfig invalid;
        invalid.max_frame_delta_seconds = 0.0;
        Application app(backend, invalid);
        require(!app.run(), "invalid configuration rejected");
        require(app.state() == ApplicationState::Failed, "invalid config failure state");
        require(!backend.initialize_called, "invalid config fails before backend initialization");
    }

    {
        FakeBackend backend;
        Application* app_ptr = nullptr;
        int updates = 0;
        ApplicationCallbacks callbacks;
        callbacks.on_update = [&](double) {
            if (++updates == 1) app_ptr->request_stop();
        };
        Application app(backend, {}, callbacks);
        app_ptr = &app;
        require(app.run(), "thread-safe stop request exits loop");
        require(updates == 1, "stop request prevents another update");
        require(app.state() == ApplicationState::Stopped, "requested stop state");
    }

    std::cout << "Dream Engine platform application tests passed." << std::endl;
    return EXIT_SUCCESS;
}
