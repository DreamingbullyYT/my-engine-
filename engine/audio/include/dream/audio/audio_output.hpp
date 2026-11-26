#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>

namespace dream::audio {

struct AudioOutputConfig final {
    std::uint32_t sample_rate{48000U};
    std::uint8_t channels{2U};
    std::uint32_t frames_per_callback{192U};
};

using AudioRenderCallback = void (*)(
    void* context,std::span<float> interleaved_output,
    std::size_t frame_count,std::uint32_t sample_rate) noexcept;

class IAudioOutput {
public:
    virtual ~IAudioOutput()=default;
    IAudioOutput()=default;
    IAudioOutput(const IAudioOutput&)=delete;
    IAudioOutput& operator=(const IAudioOutput&)=delete;

    [[nodiscard]] virtual bool start(
        AudioOutputConfig config,AudioRenderCallback callback,
        void* callback_context,std::string& error)=0;
    virtual void stop() noexcept=0;
    [[nodiscard]] virtual bool running() const noexcept=0;
    [[nodiscard]] virtual AudioOutputConfig config() const noexcept=0;
};

} // namespace dream::audio
