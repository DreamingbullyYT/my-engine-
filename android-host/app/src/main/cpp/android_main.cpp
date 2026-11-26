#include <dream/platform/android/game_activity_backend.hpp>
#include <dream/platform/application.hpp>
#include <dream/platform/window.hpp>

#if defined(__ANDROID__)
#include <android/log.h>

#include <memory>
#include <string>
#include <utility>

namespace {
constexpr const char* kLogTag = "DreamEngineHost";

void log_error(const std::string& message) noexcept {
    __android_log_print(ANDROID_LOG_ERROR, kLogTag, "%s", message.c_str());
}
}

extern "C" void android_main(android_app* app) {
    if (app == nullptr) return;

    dream::platform::AndroidGameActivityBackend backend(app, 16);
    dream::platform::WindowId window{};
    dream::platform::Application* application = nullptr;

    dream::platform::ApplicationCallbacks callbacks;
    callbacks.on_initialize = [&]() {
        dream::platform::WindowConfig config;
        config.title = "Dream Engine";
        config.width = 1280;
        config.height = 720;
        std::string error;
        if (!backend.create_window(config, window, error)) {
            log_error(error);
            return false;
        }
        return true;
    };
    callbacks.on_update = [&](double) {
        dream::platform::WindowEvent window_event;
        while (backend.poll_window_event(window_event)) {
            if (window_event.type == dream::platform::WindowEventType::CloseRequested &&
                application != nullptr) {
                application->request_stop();
            }
        }

        // Consume and release all queued input events in this host smoke test.
        dream::platform::InputEvent input_event;
        while (backend.poll_input_event(input_event)) {
        }
    };
    callbacks.on_shutdown = [&]() {
        if (window) backend.destroy_window(window);
        window = {};
    };

    dream::platform::Application host(
        backend,
        dream::platform::ApplicationConfig{
            .name = "Dream Engine Android Host",
            .max_frame_delta_seconds = 0.25
        },
        std::move(callbacks));
    application = &host;
    if (!host.run() && !host.error().empty()) log_error(host.error());
}
#endif
