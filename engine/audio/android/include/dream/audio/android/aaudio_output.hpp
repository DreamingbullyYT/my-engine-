#pragma once

#if defined(__ANDROID__)
#include <dream/audio/audio_output.hpp>
#include <aaudio/AAudio.h>

#include <atomic>
#include <mutex>

namespace dream::audio {

class AndroidAAudioOutput final : public IAudioOutput {
public:
    AndroidAAudioOutput()=default;
    ~AndroidAAudioOutput() override;

    AndroidAAudioOutput(const AndroidAAudioOutput&)=delete;
    AndroidAAudioOutput& operator=(const AndroidAAudioOutput&)=delete;

    [[nodiscard]] bool start(
        AudioOutputConfig config,AudioRenderCallback callback,
        void* callback_context,std::string& error) override;
    void stop() noexcept override;
    [[nodiscard]] bool running() const noexcept override {
        return running_.load(std::memory_order_acquire);
    }
    [[nodiscard]] AudioOutputConfig config() const noexcept override {
        return config_;
    }
    [[nodiscard]] aaudio_result_t last_error() const noexcept {
        return last_error_.load(std::memory_order_acquire);
    }

private:
    static aaudio_data_callback_result_t on_data(
        AAudioStream* stream,void* user_data,void* audio_data,
        std::int32_t num_frames) noexcept;
    static void on_error(
        AAudioStream* stream,void* user_data,
        aaudio_result_t error) noexcept;

    mutable std::mutex lifecycle_mutex_;
    AAudioStream* stream_{nullptr};
    AudioOutputConfig config_{};
    AudioRenderCallback callback_{nullptr};
    void* callback_context_{nullptr};
    std::atomic<bool> running_{false};
    std::atomic<aaudio_result_t> last_error_{AAUDIO_OK};
};

} // namespace dream::audio
#endif
