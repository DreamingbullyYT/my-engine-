#pragma once

#include <dream/core/handle.hpp>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <map>
#include <mutex>
#include <optional>
#include <span>
#include <utility>
#include <vector>

namespace dream::audio {

struct AudioClipTag;
struct AudioVoiceTag;
using AudioClipHandle = core::Handle<AudioClipTag>;
using AudioVoiceHandle = core::Handle<AudioVoiceTag>;

enum class AudioError : std::uint8_t {
    None, InvalidClip, InvalidVoiceSettings, InvalidOutput,
    InvalidHandle, ClipInUse, CapacityExhausted
};

struct AudioClip final {
    // Interleaved float PCM. Channels must be one (mono) or two (stereo).
    std::uint32_t sample_rate{48000};
    std::uint8_t channels{2};
    std::vector<float> samples;

    [[nodiscard]] std::size_t frame_count() const noexcept {
        return channels==0U?0U:samples.size()/channels;
    }
    [[nodiscard]] bool valid() const noexcept {
        if(sample_rate<8000U||sample_rate>192000U||
           (channels!=1U&&channels!=2U)||samples.empty()||
           samples.size()%channels!=0U) return false;
        return std::all_of(samples.begin(),samples.end(),
            [](float value){return std::isfinite(value);});
    }
};

struct AudioVoiceSettings final {
    float volume{1.0F};
    float pitch{1.0F};
    float pan{0.0F};
    bool looping{false};
};

struct AudioVoiceState final {
    AudioVoiceHandle handle{};
    AudioClipHandle clip{};
    double cursor_frames{0.0};
    AudioVoiceSettings settings{};
    bool playing{false};
};

struct AudioOperationResult final {
    AudioError error{AudioError::None};
    [[nodiscard]] explicit operator bool() const noexcept {
        return error==AudioError::None;
    }
};

// Deterministic software PCM mixer. It supports mono/stereo float clips,
// linear resampling, pitch, volume, pan, looping and up to max_voices voices.
// Public operations are synchronized; the initial implementation uses a mutex
// in the render callback, so it is functionally safe but not hard-real-time safe.
class AudioMixer final {
public:
    static constexpr std::size_t max_voices=128U;

    AudioMixer()=default;
    AudioMixer(const AudioMixer&)=delete;
    AudioMixer& operator=(const AudioMixer&)=delete;

    [[nodiscard]] std::pair<AudioClipHandle,AudioError> add_clip(AudioClip clip) {
        if(!clip.valid()) return {{},AudioError::InvalidClip};
        std::lock_guard lock(mutex_);
        const auto handle=clip_handles_.allocate();
        if(!handle) return {{},AudioError::CapacityExhausted};
        try {
            const auto [it,inserted]=clips_.emplace(handle.index(),std::move(clip));
            (void)it;
            if(!inserted) {
                (void)clip_handles_.release(handle);
                return {{},AudioError::CapacityExhausted};
            }
        } catch(...) {
            (void)clip_handles_.release(handle);
            throw;
        }
        return {handle,AudioError::None};
    }

    [[nodiscard]] AudioOperationResult remove_clip(AudioClipHandle handle) {
        std::lock_guard lock(mutex_);
        if(!find_clip(handle)) return {AudioError::InvalidHandle};
        for(const auto& [index,voice]:voices_) {
            (void)index;
            if(voice.clip==handle) return {AudioError::ClipInUse};
        }
        if(!clip_handles_.release(handle)) return {AudioError::InvalidHandle};
        clips_.erase(handle.index());
        return {};
    }

    [[nodiscard]] std::pair<AudioVoiceHandle,AudioError> play(
        AudioClipHandle clip,AudioVoiceSettings settings={}) {
        if(!valid_voice_settings(settings))
            return {{},AudioError::InvalidVoiceSettings};
        std::lock_guard lock(mutex_);
        if(!find_clip(clip)) return {{},AudioError::InvalidHandle};
        if(voices_.size()>=max_voices) return {{},AudioError::CapacityExhausted};
        const auto handle=voice_handles_.allocate();
        if(!handle) return {{},AudioError::CapacityExhausted};
        try {
            const auto [it,inserted]=voices_.emplace(handle.index(),
                Voice{handle,clip,0.0,settings});
            (void)it;
            if(!inserted) {
                (void)voice_handles_.release(handle);
                return {{},AudioError::CapacityExhausted};
            }
        } catch(...) {
            (void)voice_handles_.release(handle);
            throw;
        }
        return {handle,AudioError::None};
    }

    [[nodiscard]] AudioOperationResult stop(AudioVoiceHandle handle) {
        std::lock_guard lock(mutex_);
        if(!find_voice(handle)||!voice_handles_.release(handle))
            return {AudioError::InvalidHandle};
        voices_.erase(handle.index());
        return {};
    }

    [[nodiscard]] AudioOperationResult set_settings(
        AudioVoiceHandle handle,AudioVoiceSettings settings) {
        if(!valid_voice_settings(settings))
            return {AudioError::InvalidVoiceSettings};
        std::lock_guard lock(mutex_);
        auto* voice=find_voice(handle);
        if(!voice) return {AudioError::InvalidHandle};
        voice->settings=settings;
        return {};
    }

    [[nodiscard]] std::optional<AudioVoiceState> voice_state(
        AudioVoiceHandle handle) const {
        std::lock_guard lock(mutex_);
        const auto* voice=find_voice(handle);
        if(!voice) return std::nullopt;
        return AudioVoiceState{voice->handle,voice->clip,voice->cursor,
            voice->settings,true};
    }

    [[nodiscard]] std::size_t voice_count() const noexcept {
        std::lock_guard lock(mutex_);
        return voices_.size();
    }
    [[nodiscard]] std::size_t clip_count() const noexcept {
        std::lock_guard lock(mutex_);
        return clips_.size();
    }

    // Writes exactly frame_count*2 interleaved stereo float samples. The output
    // span is untouched on invalid arguments. A zero-frame request is a no-op.
    [[nodiscard]] AudioError mix_stereo(
        std::span<float> output,std::size_t frame_count,
        std::uint32_t output_sample_rate) {
        if(output_sample_rate<8000U||output_sample_rate>192000U||
           frame_count>std::numeric_limits<std::size_t>::max()/2U||
           output.size()!=frame_count*2U)
            return AudioError::InvalidOutput;
        std::lock_guard lock(mutex_);
        std::fill(output.begin(),output.end(),0.0F);
        if(frame_count==0U) return AudioError::None;

        for(auto it=voices_.begin();it!=voices_.end();) {
            auto& voice=it->second;
            const auto* clip=find_clip(voice.clip);
            if(clip==nullptr) {
                (void)voice_handles_.release(voice.handle);
                it=voices_.erase(it);
                continue;
            }
            const double step=static_cast<double>(clip->sample_rate)/
                static_cast<double>(output_sample_rate)*
                static_cast<double>(voice.settings.pitch);
            const double clip_frames=static_cast<double>(clip->frame_count());
            const float left_pan=voice.settings.pan>0.0F
                ?1.0F-voice.settings.pan:1.0F;
            const float right_pan=voice.settings.pan<0.0F
                ?1.0F+voice.settings.pan:1.0F;
            bool finished=false;

            for(std::size_t frame=0;frame<frame_count;++frame) {
                if(voice.cursor>=clip_frames) {
                    if(voice.settings.looping) {
                        voice.cursor=std::fmod(voice.cursor,clip_frames);
                    } else {
                        finished=true;
                        break;
                    }
                }
                const auto first=static_cast<std::size_t>(voice.cursor);
                const auto second=std::min(first+1U,clip->frame_count()-1U);
                const float fraction=static_cast<float>(voice.cursor-
                    static_cast<double>(first));
                const auto sample=[&](std::size_t at,std::size_t channel) {
                    const std::size_t source_channel=clip->channels==1U?0U:channel;
                    return clip->samples[at*clip->channels+source_channel];
                };
                const float left=sample(first,0U)+
                    (sample(second,0U)-sample(first,0U))*fraction;
                const float right=sample(first,1U)+
                    (sample(second,1U)-sample(first,1U))*fraction;
                const float gain=voice.settings.volume;
                output[frame*2U]+=left*gain*left_pan;
                output[frame*2U+1U]+=right*gain*right_pan;
                voice.cursor+=step;
            }

            if(finished||(voice.cursor>=clip_frames&&!voice.settings.looping)) {
                (void)voice_handles_.release(voice.handle);
                it=voices_.erase(it);
            } else {
                ++it;
            }
        }

        for(auto& value:output) {
            if(!std::isfinite(value)) value=0.0F;
            else value=std::clamp(value,-1.0F,1.0F);
        }
        return AudioError::None;
    }

private:
    struct Voice final {
        AudioVoiceHandle handle{};
        AudioClipHandle clip{};
        double cursor{0.0};
        AudioVoiceSettings settings{};
    };

    [[nodiscard]] static bool valid_voice_settings(
        AudioVoiceSettings s) noexcept {
        return std::isfinite(s.volume)&&s.volume>=0.0F&&s.volume<=4.0F&&
            std::isfinite(s.pitch)&&s.pitch>=0.25F&&s.pitch<=4.0F&&
            std::isfinite(s.pan)&&s.pan>=-1.0F&&s.pan<=1.0F;
    }
    [[nodiscard]] AudioClip* find_clip(AudioClipHandle handle) noexcept {
        if(!handle) return nullptr;
        const auto it=clips_.find(handle.index());
        return it!=clips_.end()&&clip_handles_.is_alive(handle)?&it->second:nullptr;
    }
    [[nodiscard]] const AudioClip* find_clip(AudioClipHandle handle) const noexcept {
        if(!handle) return nullptr;
        const auto it=clips_.find(handle.index());
        return it!=clips_.end()&&clip_handles_.is_alive(handle)?&it->second:nullptr;
    }
    [[nodiscard]] Voice* find_voice(AudioVoiceHandle handle) noexcept {
        if(!handle) return nullptr;
        const auto it=voices_.find(handle.index());
        return it!=voices_.end()&&it->second.handle==handle?&it->second:nullptr;
    }
    [[nodiscard]] const Voice* find_voice(AudioVoiceHandle handle) const noexcept {
        if(!handle) return nullptr;
        const auto it=voices_.find(handle.index());
        return it!=voices_.end()&&it->second.handle==handle?&it->second:nullptr;
    }

    mutable std::mutex mutex_;
    core::HandlePool<AudioClipTag> clip_handles_;
    core::HandlePool<AudioVoiceTag> voice_handles_;
    std::map<std::uint32_t,AudioClip> clips_;
    std::map<std::uint32_t,Voice> voices_;
};

} // namespace dream::audio
