#include <dream/audio/audio_mixer.hpp>

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <initializer_list>
#include <limits>
#include <string_view>
#include <vector>

namespace {
using namespace dream::audio;
void require(bool ok,std::string_view message) {
    if(!ok){std::cerr<<"FAILED: "<<message<<'\n';std::exit(EXIT_FAILURE);}
}
bool near(float a,float b,float e=1.0e-4F){return std::abs(a-b)<=e;}
AudioClip mono(std::initializer_list<float> samples,std::uint32_t rate=48000) {
    AudioClip clip;clip.sample_rate=rate;clip.channels=1;clip.samples=samples;return clip;
}
}

int main() {
    AudioMixer mixer;
    const auto invalid=mixer.add_clip(AudioClip{.sample_rate=0});
    require(!invalid.second&&invalid.second==AudioError::InvalidClip,
        "invalid sample rate is rejected");

    const auto added=mixer.add_clip(mono({0.25F,0.5F,-0.5F,-0.25F}));
    require(added.second==AudioError::None&&static_cast<bool>(added.first),
        "valid mono PCM clip is registered");
    require(mixer.clip_count()==1,"clip count increments");

    const auto voice=mixer.play(added.first);
    require(voice.second==AudioError::None&&static_cast<bool>(voice.first),
        "clip can be played as a voice");
    require(mixer.remove_clip(added.first).error==AudioError::ClipInUse,
        "active clip cannot be removed");

    std::vector<float> output(8U);
    require(mixer.mix_stereo(output,4U,48000U)==AudioError::None,
        "mixer renders the requested stereo frames");
    require(near(output[0],0.25F)&&near(output[1],0.25F)&&
            near(output[2],0.5F)&&near(output[3],0.5F),
        "mono input is duplicated to left and right channels");
    require(near(output[4],-0.5F)&&near(output[5],-0.5F),
        "PCM polarity is preserved");
    require(mixer.voice_count()==0&&!mixer.voice_state(voice.first),
        "completed voice is released automatically");
    require(static_cast<bool>(mixer.remove_clip(added.first)),
        "completed clip can be removed");

    const auto stereo=mixer.add_clip(AudioClip{
        .sample_rate=48000,.channels=2,
        .samples={1.0F,0.5F,0.5F,1.0F}});
    require(stereo.second==AudioError::None,"stereo PCM clip registers");
    auto left=mixer.play(stereo.first,AudioVoiceSettings{
        .volume=0.5F,.pitch=1.0F,.pan=-1.0F,.looping=true});
    require(left.second==AudioError::None,"left-panned looping voice starts");
    std::vector<float> panned(4U);
    require(mixer.mix_stereo(panned,2U,48000U)==AudioError::None,
        "stereo voice mixes");
    require(near(panned[0],0.5F)&&near(panned[1],0.0F)&&
            near(panned[2],0.25F)&&near(panned[3],0.0F),
        "pan and volume are applied independently to channels");
    require(mixer.voice_state(left.first)->cursor_frames==2.0,
        "voice cursor advances by output frames");
    require(static_cast<bool>(mixer.set_settings(left.first,AudioVoiceSettings{
        .volume=2.0F,.pitch=2.0F,.pan=1.0F,.looping=true})),
        "voice playback settings can be changed");

    std::vector<float> resampled(4U);
    require(mixer.mix_stereo(resampled,2U,96000U)==AudioError::None,
        "mixer resamples when output sample rate differs from clip");
    require(mixer.voice_state(left.first)->cursor_frames==3.0,
        "resampling advances the source cursor by the rate ratio");

    require(!mixer.set_settings(left.first,AudioVoiceSettings{
        .volume=1,.pitch=0.1F,.pan=0,.looping=false}),
        "out-of-range pitch is rejected");
    require(static_cast<bool>(mixer.stop(left.first)),"voice can be stopped");
    require(!mixer.stop(left.first),"stale voice handle is rejected");
    require(static_cast<bool>(mixer.remove_clip(stereo.first)),
        "unused stereo clip can be removed");

    const auto looping=mixer.add_clip(mono({0.75F,0.25F}));
    const auto loop_voice=mixer.play(looping.first,AudioVoiceSettings{
        .volume=1,.pitch=1,.pan=0,.looping=true});
    std::vector<float> loop_output(12U);
    require(mixer.mix_stereo(loop_output,6U,48000U)==AudioError::None,
        "looping voice renders multiple cycles");
    require(mixer.voice_count()==1&&mixer.voice_state(loop_voice.first),
        "looping voice remains active");
    require(static_cast<bool>(mixer.stop(loop_voice.first)),
        "looping voice stops");
    require(static_cast<bool>(mixer.remove_clip(looping.first)),
        "loop clip is released");

    std::vector<float> preserved{7.0F,8.0F};
    require(mixer.mix_stereo(preserved,2U,0U)==AudioError::InvalidOutput&&
            preserved[0]==7.0F&&preserved[1]==8.0F,
        "invalid output request leaves destination untouched");
    return EXIT_SUCCESS;
}
