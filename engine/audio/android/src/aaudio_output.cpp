#include <dream/audio/android/aaudio_output.hpp>

#if defined(__ANDROID__)
#include <algorithm>
#include <cstdint>
#include <string>

namespace dream::audio {
namespace {
[[nodiscard]] bool valid_config(AudioOutputConfig c) noexcept {
    return c.sample_rate>=8000U&&c.sample_rate<=192000U&&
        c.channels==2U&&c.frames_per_callback>=48U&&
        c.frames_per_callback<=4096U;
}
void silence(void* data,std::int32_t frames,std::uint8_t channels) noexcept {
    if(data==nullptr||frames<=0||channels==0U) return;
    auto* samples=static_cast<float*>(data);
    const auto count=static_cast<std::size_t>(frames)*
        static_cast<std::size_t>(channels);
    std::fill_n(samples,count,0.0F);
}
}
AndroidAAudioOutput::~AndroidAAudioOutput() { stop(); }

bool AndroidAAudioOutput::start(
    AudioOutputConfig config,AudioRenderCallback callback,
    void* callback_context,std::string& error) {
    std::lock_guard lock(lifecycle_mutex_);
    if(stream_!=nullptr||running_.load(std::memory_order_acquire)) {
        error="AAudio output is already running";
        return false;
    }
    if(!valid_config(config)||callback==nullptr) {
        error="AAudio requires stereo float output, a supported sample rate and callback";
        return false;
    }

    AAudioStreamBuilder* builder=nullptr;
    auto result=AAudio_createStreamBuilder(&builder);
    if(result!=AAUDIO_OK||builder==nullptr) {
        error=std::string("AAudio_createStreamBuilder: ")+AAudio_convertResultToText(result);
        last_error_.store(result,std::memory_order_release);
        return false;
    }

    AAudioStreamBuilder_setDirection(builder,AAUDIO_DIRECTION_OUTPUT);
    AAudioStreamBuilder_setFormat(builder,AAUDIO_FORMAT_PCM_FLOAT);
    AAudioStreamBuilder_setChannelCount(builder,2);
    AAudioStreamBuilder_setSampleRate(builder,static_cast<std::int32_t>(config.sample_rate));
    AAudioStreamBuilder_setPerformanceMode(builder,AAUDIO_PERFORMANCE_MODE_LOW_LATENCY);
    AAudioStreamBuilder_setSharingMode(builder,AAUDIO_SHARING_MODE_SHARED);
    AAudioStreamBuilder_setFramesPerDataCallback(
        builder,static_cast<std::int32_t>(config.frames_per_callback));
    AAudioStreamBuilder_setDataCallback(builder,&AndroidAAudioOutput::on_data,this);
    AAudioStreamBuilder_setErrorCallback(builder,&AndroidAAudioOutput::on_error,this);

    AAudioStream* opened=nullptr;
    result=AAudioStreamBuilder_openStream(builder,&opened);
    AAudioStreamBuilder_delete(builder);
    if(result!=AAUDIO_OK||opened==nullptr) {
        error=std::string("AAudioStreamBuilder_openStream: ")+
            AAudio_convertResultToText(result);
        last_error_.store(result,std::memory_order_release);
        return false;
    }

    stream_=opened;
    config_=config;
    callback_context_.store(callback_context,std::memory_order_release);
    callback_.store(callback,std::memory_order_release);
    running_.store(true,std::memory_order_release);
    result=AAudioStream_requestStart(stream_);
    if(result!=AAUDIO_OK) {
        running_.store(false,std::memory_order_release);
        error=std::string("AAudioStream_requestStart: ")+
            AAudio_convertResultToText(result);
        last_error_.store(result,std::memory_order_release);
        AAudioStream_close(stream_);
        stream_=nullptr;
        callback_.store(nullptr,std::memory_order_release);
        callback_context_.store(nullptr,std::memory_order_release);
        return false;
    }

    last_error_.store(AAUDIO_OK,std::memory_order_release);
    error.clear();
    return true;
}

void AndroidAAudioOutput::stop() noexcept {
    std::lock_guard lock(lifecycle_mutex_);
    running_.store(false,std::memory_order_release);
    if(stream_==nullptr) {
        callback_.store(nullptr,std::memory_order_release);
        callback_context_.store(nullptr,std::memory_order_release);
        return;
    }
    (void)AAudioStream_requestStop(stream_);
    (void)AAudioStream_close(stream_);
    stream_=nullptr;
    callback_.store(nullptr,std::memory_order_release);
    callback_context_.store(nullptr,std::memory_order_release);
}

aaudio_data_callback_result_t AndroidAAudioOutput::on_data(
    AAudioStream* stream,void* user_data,void* audio_data,
    std::int32_t num_frames) noexcept {
    if(user_data==nullptr||audio_data==nullptr||num_frames<=0)
        return AAUDIO_CALLBACK_RESULT_STOP;
    auto& output=*static_cast<AndroidAAudioOutput*>(user_data);
    const auto channels=AAudioStream_getChannelCount(stream);
    const auto rate=AAudioStream_getSampleRate(stream);
    if(channels!=2||rate<=0||!output.running_.load(std::memory_order_acquire)) {
        silence(audio_data,num_frames,channels>0?static_cast<std::uint8_t>(channels):2U);
        return AAUDIO_CALLBACK_RESULT_CONTINUE;
    }
    const auto frames=static_cast<std::size_t>(num_frames);
    auto samples=std::span<float>(static_cast<float*>(audio_data),frames*2U);
    const auto callback=output.callback_.load(std::memory_order_acquire);
    void* const callback_context=
        output.callback_context_.load(std::memory_order_acquire);
    if(callback==nullptr) {
        std::fill(samples.begin(),samples.end(),0.0F);
        return AAUDIO_CALLBACK_RESULT_CONTINUE;
    }
    callback(callback_context,samples,frames,static_cast<std::uint32_t>(rate));
    return AAUDIO_CALLBACK_RESULT_CONTINUE;
}

void AndroidAAudioOutput::on_error(
    AAudioStream*,void* user_data,aaudio_result_t error) noexcept {
    if(user_data==nullptr) return;
    auto& output=*static_cast<AndroidAAudioOutput*>(user_data);
    output.last_error_.store(error,std::memory_order_release);
    output.running_.store(false,std::memory_order_release);
}

} // namespace dream::audio
#endif
