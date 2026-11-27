#include <dream/core/log.hpp>
#include <dream/platform/android/game_activity_backend.hpp>
#include <dream/platform/application.hpp>
#include <dream/platform/window.hpp>
#include <dream/render/wgpu_native/wgpu_native_backend.hpp>
#include <dream/runtime/engine_runtime.hpp>
#include <dream/audio/android/aaudio_output.hpp>

#if defined(__ANDROID__)
#include <android/native_activity.h>
#include <android/log.h>

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

extern "C" void android_main(android_app* app) {
    __android_log_write(ANDROID_LOG_INFO, "DreamEngineBoot",
                        "android_main entered (wgpu-native host)");
    if (app == nullptr || app->activity == nullptr) {
        __android_log_write(ANDROID_LOG_ERROR, "DreamEngineBoot",
                            "android_main received an invalid android_app/activity");
        return;
    }

    using dream::core::LogLevel;
    using dream::core::Logger;

    Logger logger;
    std::vector<Logger::Sink> sinks;
    sinks.emplace_back(dream::core::make_android_logcat_sink("DreamEngine"));

    std::string log_path;
    const char* log_directory = app->activity->externalDataPath;
    if (log_directory == nullptr || log_directory[0] == '\0')
        log_directory = app->activity->internalDataPath;
    if (log_directory != nullptr && log_directory[0] != '\0') {
        log_path = std::string(log_directory) + "/logs/engine.log";
        sinks.emplace_back(dream::core::make_file_log_sink(
            log_path, dream::core::FileLogSinkOptions{8U * 1024U * 1024U, 3U, true}));
    }
    logger.set_sink(dream::core::make_combined_log_sink(std::move(sinks)));
    logger.set_minimum_level(LogLevel::debug);
    logger.write(LogLevel::info, "platform.android",
                 "Dream Engine wgpu-native diagnostic host; log_file=" +
                 (log_path.empty() ? std::string("unavailable (Logcat only)") : log_path));

    dream::platform::AndroidGameActivityBackend platform_backend(app, 16);
    dream::render::WgpuNativeBackend render_backend;
    dream::runtime::EngineRuntimeConfig runtime_config;
    runtime_config.scene_name = "WgpuAndroidClearTest";
    runtime_config.logger = &logger;
    dream::runtime::EngineRuntime engine(
        platform_backend, render_backend, runtime_config);

    dream::audio::AndroidAAudioOutput audio_output;
    bool audio_started = false;
    bool runtime_initialized = false;
    bool renderer_ready = false;
    bool application_suspended = false;
    std::uint64_t frames_presented = 0;
    dream::platform::WindowId window{};
    dream::platform::Application* application = nullptr;
    std::vector<dream::platform::InputEvent> input_events;
    input_events.reserve(128U);

    const auto start_audio = [&]() {
        if (audio_started) return;
        std::string error;
        if (audio_output.start(
                dream::audio::AudioOutputConfig{},
                &dream::audio::AudioMixer::render_callback,
                &engine.audio(), error)) {
            audio_started = true;
            logger.write(LogLevel::info, "audio.aaudio",
                         "AAudio output started with the engine software mixer");
        } else {
            logger.write(LogLevel::warning, "audio.aaudio",
                         "AAudio output could not start: " + error);
        }
    };

    const auto initialize_renderer = [&]() {
        if (!runtime_initialized) {
            const auto result = engine.initialize(window);
            if (!result) {
                logger.write(LogLevel::critical, "runtime.lifecycle",
                    "EngineRuntime/wgpu initialization failed: " +
                    result.render.message);
                return false;
            }
            runtime_initialized = true;
        } else {
            const auto result = engine.render_device().initialize(window);
            if (!result) {
                logger.write(LogLevel::critical, "render.device",
                    "wgpu surface recreation failed: " + result.message);
                return false;
            }
        }
        renderer_ready = engine.render_device().initialized();
        return renderer_ready;
    };

    dream::platform::ApplicationCallbacks callbacks;
    callbacks.on_initialize = [&]() {
        __android_log_write(ANDROID_LOG_INFO, "DreamEngineBoot",
                            "Application on_initialize callback entered");
        dream::platform::WindowConfig config;
        config.title = "Dream Engine";
        config.width = 1280;
        config.height = 720;
        std::string error;
        if (!platform_backend.create_window(config, window, error)) {
            logger.write(LogLevel::critical, "platform.window",
                         "GameActivity window creation failed: " + error);
            return false;
        }
        logger.write(LogLevel::info, "platform.window",
                     "GameActivity window created; waiting for Android surface");
        return true;
    };

    callbacks.on_update = [&](double delta_seconds) {
        dream::platform::WindowEvent window_event;
        while (platform_backend.poll_window_event(window_event)) {
            switch (window_event.type) {
            case dream::platform::WindowEventType::SurfaceCreated:
                __android_log_write(ANDROID_LOG_INFO, "DreamEngineBoot",
                                    "GameActivity surface created; initializing wgpu");
                if (!initialize_renderer()) {
                    logger.write(LogLevel::critical, "runtime.lifecycle",
                                 "Could not initialize the wgpu Android surface");
                    if (application != nullptr) application->request_stop();
                } else {
                    __android_log_write(ANDROID_LOG_INFO, "DreamEngineBoot",
                                        "wgpu adapter, device and surface configured");
                    logger.write(LogLevel::info, "render.wgpu",
                                 "wgpu-native initialized; clear/present test is active");
                    start_audio();
                }
                break;
            case dream::platform::WindowEventType::SurfaceDestroyed:
                logger.write(LogLevel::warning, "platform.surface",
                             "Android surface destroyed; releasing wgpu device and surface");
                if (engine.render_device().initialized())
                    engine.render_device().shutdown();
                renderer_ready = false;
                break;
            case dream::platform::WindowEventType::Resized:
                logger.write(LogLevel::debug, "platform.surface",
                             "Android surface metrics changed; wgpu will reconfigure before rendering");
                break;
            case dream::platform::WindowEventType::Suspended:
                application_suspended = true;
                if (audio_output.running()) audio_output.stop();
                audio_started = false;
                logger.write(LogLevel::info, "platform.lifecycle",
                             "Android app suspended; audio output stopped");
                break;
            case dream::platform::WindowEventType::Resumed:
                application_suspended = false;
                logger.write(LogLevel::info, "platform.lifecycle",
                             "Android app resumed");
                if (renderer_ready) start_audio();
                break;
            case dream::platform::WindowEventType::CloseRequested:
                if (application != nullptr) application->request_stop();
                break;
            case dream::platform::WindowEventType::FocusGained:
            case dream::platform::WindowEventType::FocusLost:
            case dream::platform::WindowEventType::MemoryLow:
            case dream::platform::WindowEventType::Minimized:
            case dream::platform::WindowEventType::Restored:
                break;
            }
        }

        input_events.clear();
        dream::platform::InputEvent input_event;
        while (platform_backend.poll_input_event(input_event))
            input_events.push_back(input_event);

        if (renderer_ready && !application_suspended) {
            const auto update = engine.update(delta_seconds, input_events);
            if (!update) {
                logger.write(LogLevel::error, "runtime.update",
                    "Engine update failed; error=" +
                    std::to_string(static_cast<unsigned>(update.error)));
            }

            // A vivid clear colour makes native surface presentation observable
            // on the first phone test. 2D and 3D draws are added in later stages.
            const auto begin = engine.begin_render_frame(
                {1.0F, 0.0F, 1.0F, 1.0F});
            if (!begin) {
                logger.write(LogLevel::error, "render.wgpu",
                             "Could not begin wgpu test frame: " +
                             begin.render.message);
            } else {
                const auto rendered = engine.render_frame();
                if (!rendered) {
                    logger.write(LogLevel::error, "render.wgpu",
                        "wgpu frame submission failed: " + rendered.render.message);
                } else if ((frames_presented++ % 120U) == 0U) {
                    logger.write(LogLevel::info, "render.wgpu",
                        "wgpu clear/present frames submitted: " +
                        std::to_string(frames_presented));
                }
            }
        }

        if (audio_started && !audio_output.running()) {
            logger.write(LogLevel::error, "audio.aaudio",
                         "AAudio stream stopped unexpectedly");
            audio_started = false;
        }
    };

    callbacks.on_shutdown = [&]() {
        logger.write(LogLevel::info, "platform.lifecycle",
                     "Android wgpu host shutdown requested");
        audio_output.stop();
        audio_started = false;
        engine.shutdown();
        if (window) platform_backend.destroy_window(window);
        window = {};
    };

    dream::platform::Application host(
        platform_backend,
        dream::platform::ApplicationConfig{
            .name = "Dream Engine Android wgpu Host",
            .max_frame_delta_seconds = 0.25
        },
        std::move(callbacks));
    application = &host;
    __android_log_write(ANDROID_LOG_INFO, "DreamEngineBoot",
                        "Entering native Application::run loop");
    if (!host.run()) {
        logger.write(LogLevel::critical, "platform.application",
                     "Application loop failed: " + host.error());
    } else {
        logger.write(LogLevel::info, "platform.application",
                     "Application loop stopped cleanly");
    }
}
#endif
