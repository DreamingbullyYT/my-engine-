#include <dream/render/wgpu_native/wgpu_native_backend.hpp>

#if defined(__ANDROID__)

#include <wgpu.h>

#include <android/log.h>

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <thread>

namespace dream::render {
namespace {

constexpr char kLogTag[] = "DreamEngineWgpu";
constexpr auto kRequestTimeout = std::chrono::seconds(15);

std::string message_string(WGPUStringView value) {
    if (value.data == nullptr) return {};
    const std::size_t length =
        value.length == WGPU_STRLEN ? std::strlen(value.data) : value.length;
    return std::string(value.data, length);
}

void wgpu_log_callback(WGPULogLevel level, WGPUStringView message, void*) {
    const std::string text = message_string(message);
    const int priority = level == WGPULogLevel_Error
        ? ANDROID_LOG_ERROR
        : (level == WGPULogLevel_Warn ? ANDROID_LOG_WARN : ANDROID_LOG_INFO);
    __android_log_write(priority, kLogTag, text.c_str());
}

void uncaptured_error_callback(
    WGPUDevice const*, WGPUErrorType type, WGPUStringView message,
    void*, void*) {
    const std::string text =
        "Uncaptured WebGPU error type=" +
        std::to_string(static_cast<unsigned>(type)) + ": " +
        message_string(message);
    __android_log_write(ANDROID_LOG_ERROR, kLogTag, text.c_str());
}

struct AdapterRequest final {
    bool completed{false};
    WGPURequestAdapterStatus status{WGPURequestAdapterStatus_Unknown};
    WGPUAdapter adapter{nullptr};
    std::string message;
};

void adapter_request_callback(
    WGPURequestAdapterStatus status, WGPUAdapter adapter,
    WGPUStringView message, void* userdata1, void*) {
    auto* result = static_cast<AdapterRequest*>(userdata1);
    if (result == nullptr) return;
    result->status = status;
    result->adapter = adapter;
    result->message = message_string(message);
    result->completed = true;
}

struct DeviceRequest final {
    bool completed{false};
    WGPURequestDeviceStatus status{WGPURequestDeviceStatus_Unknown};
    WGPUDevice device{nullptr};
    std::string message;
};

void device_request_callback(
    WGPURequestDeviceStatus status, WGPUDevice device,
    WGPUStringView message, void* userdata1, void*) {
    auto* result = static_cast<DeviceRequest*>(userdata1);
    if (result == nullptr) return;
    result->status = status;
    result->device = device;
    result->message = message_string(message);
    result->completed = true;
}

template <typename Result>
bool wait_for_request(
    WGPUInstance instance, Result& result, const char* operation,
    std::string& error) {
    const auto deadline = std::chrono::steady_clock::now() + kRequestTimeout;
    while (!result.completed && std::chrono::steady_clock::now() < deadline) {
        wgpuInstanceProcessEvents(instance);
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    if (!result.completed) {
        error = std::string(operation) + " timed out after 15 seconds";
        return false;
    }
    return true;
}

bool configure_surface(
    WGPUSurface surface, WGPUDevice device,
    WGPUTextureFormat format, WGPUCompositeAlphaMode alpha_mode,
    const RenderSurfaceInfo& info, WGPUSurfaceConfiguration& configuration,
    std::string& error) {
    if (!info.pixel_extent.valid()) {
        error = "wgpu surface cannot be configured with a zero pixel extent";
        return false;
    }

    configuration = WGPU_SURFACE_CONFIGURATION_INIT;
    configuration.device = device;
    configuration.format = format;
    configuration.usage = WGPUTextureUsage_RenderAttachment;
    configuration.width = info.pixel_extent.width;
    configuration.height = info.pixel_extent.height;
    configuration.presentMode = WGPUPresentMode_Fifo;
    configuration.alphaMode = alpha_mode;
    wgpuSurfaceConfigure(surface, &configuration);
    error.clear();
    return true;
}

} // namespace

struct WgpuNativeBackend::State final {
    WGPUInstance instance{nullptr};
    WGPUSurface surface{nullptr};
    WGPUAdapter adapter{nullptr};
    WGPUDevice device{nullptr};
    WGPUQueue queue{nullptr};
    WGPUTextureFormat surface_format{WGPUTextureFormat_Undefined};
    WGPUCompositeAlphaMode alpha_mode{WGPUCompositeAlphaMode_Auto};
    WGPUSurfaceConfiguration surface_configuration{};
    RenderSurfaceInfo surface_info{};
};

WgpuNativeBackend::WgpuNativeBackend()
    : state_(std::make_unique<State>()) {}

WgpuNativeBackend::~WgpuNativeBackend() {
    shutdown();
}

bool WgpuNativeBackend::initialize(
    platform::NativeSurface native_surface,
    const RenderSurfaceInfo& surface,
    RenderCapabilities& capabilities,
    std::string& error) {
    shutdown();
    if (native_surface.kind != platform::NativeSurfaceKind::Android ||
        native_surface.window == nullptr) {
        error = "wgpu-native Android backend requires a valid ANativeWindow";
        return false;
    }
    if (!surface.pixel_extent.valid()) {
        error = "wgpu-native requires a non-zero Android framebuffer extent";
        return false;
    }

    wgpuSetLogCallback(&wgpu_log_callback, nullptr);
    wgpuSetLogLevel(WGPULogLevel_Warn);

    WGPUInstanceExtras extras{};
    extras.chain.sType = WGPUSType_InstanceExtras;
    extras.backends = WGPUInstanceBackend_Vulkan | WGPUInstanceBackend_GL;
    extras.flags = WGPUInstanceFlag_Validation;

    WGPUInstanceDescriptor instance_descriptor = WGPU_INSTANCE_DESCRIPTOR_INIT;
    instance_descriptor.nextInChain = &extras.chain;
    state_->instance = wgpuCreateInstance(&instance_descriptor);
    if (state_->instance == nullptr) {
        error = "wgpuCreateInstance returned null";
        return false;
    }

    WGPUSurfaceSourceAndroidNativeWindow android_window{};
    android_window.chain.sType =
        WGPUSType_SurfaceSourceAndroidNativeWindow;
    android_window.window = native_surface.window;

    WGPUSurfaceDescriptor surface_descriptor = WGPU_SURFACE_DESCRIPTOR_INIT;
    surface_descriptor.nextInChain = &android_window.chain;
    state_->surface =
        wgpuInstanceCreateSurface(state_->instance, &surface_descriptor);
    if (state_->surface == nullptr) {
        error = "wgpuInstanceCreateSurface returned null for ANativeWindow";
        return false;
    }

    WGPURequestAdapterOptions adapter_options =
        WGPU_REQUEST_ADAPTER_OPTIONS_INIT;
    adapter_options.compatibleSurface = state_->surface;
    adapter_options.powerPreference = WGPUPowerPreference_HighPerformance;

    AdapterRequest adapter_result;
    WGPURequestAdapterCallbackInfo adapter_callback_info =
        WGPU_REQUEST_ADAPTER_CALLBACK_INFO_INIT;
    adapter_callback_info.mode = WGPUCallbackMode_AllowProcessEvents;
    adapter_callback_info.callback = &adapter_request_callback;
    adapter_callback_info.userdata1 = &adapter_result;
    (void)wgpuInstanceRequestAdapter(
        state_->instance, &adapter_options, adapter_callback_info);

    if (!wait_for_request(
            state_->instance, adapter_result, "wgpu adapter request", error)) {
        return false;
    }
    if (adapter_result.status != WGPURequestAdapterStatus_Success ||
        adapter_result.adapter == nullptr) {
        error = "wgpu adapter request failed (status=" +
            std::to_string(static_cast<unsigned>(adapter_result.status)) +
            "): " + adapter_result.message;
        return false;
    }
    state_->adapter = adapter_result.adapter;

    WGPUAdapterInfo adapter_info = WGPU_ADAPTER_INFO_INIT;
    const WGPUStatus info_status =
        wgpuAdapterGetInfo(state_->adapter, &adapter_info);
    if (info_status == WGPUStatus_Success) {
        const std::string description = message_string(adapter_info.description);
        const std::string vendor = message_string(adapter_info.vendor);
        const std::string device = message_string(adapter_info.device);
        const std::string log_line =
            "Adapter selected: " + description + " (" + vendor + ", " + device +
            "), backend=" +
            std::to_string(static_cast<unsigned>(adapter_info.backendType));
        __android_log_write(ANDROID_LOG_INFO, kLogTag, log_line.c_str());
    }
    wgpuAdapterInfoFreeMembers(adapter_info);

    WGPUDeviceDescriptor device_descriptor = WGPU_DEVICE_DESCRIPTOR_INIT;
    device_descriptor.label = {"Dream Engine Android Device", WGPU_STRLEN};
    device_descriptor.uncapturedErrorCallbackInfo.callback =
        &uncaptured_error_callback;

    DeviceRequest device_result;
    WGPURequestDeviceCallbackInfo device_callback_info =
        WGPU_REQUEST_DEVICE_CALLBACK_INFO_INIT;
    device_callback_info.mode = WGPUCallbackMode_AllowProcessEvents;
    device_callback_info.callback = &device_request_callback;
    device_callback_info.userdata1 = &device_result;
    (void)wgpuAdapterRequestDevice(
        state_->adapter, &device_descriptor, device_callback_info);

    if (!wait_for_request(
            state_->instance, device_result, "wgpu device request", error)) {
        return false;
    }
    if (device_result.status != WGPURequestDeviceStatus_Success ||
        device_result.device == nullptr) {
        error = "wgpu device request failed (status=" +
            std::to_string(static_cast<unsigned>(device_result.status)) +
            "): " + device_result.message;
        return false;
    }
    state_->device = device_result.device;
    state_->queue = wgpuDeviceGetQueue(state_->device);
    if (state_->queue == nullptr) {
        error = "wgpuDeviceGetQueue returned null";
        return false;
    }

    WGPUSurfaceCapabilities surface_capabilities =
        WGPU_SURFACE_CAPABILITIES_INIT;
    wgpuSurfaceGetCapabilities(
        state_->surface, state_->adapter, &surface_capabilities);
    if (surface_capabilities.formatCount == 0U ||
        surface_capabilities.formats == nullptr) {
        wgpuSurfaceCapabilitiesFreeMembers(surface_capabilities);
        error = "wgpu surface reported no compatible color formats";
        return false;
    }

    state_->surface_format = surface_capabilities.formats[0];
    for (std::size_t i = 0; i < surface_capabilities.formatCount; ++i) {
        const auto candidate = surface_capabilities.formats[i];
        if (candidate == WGPUTextureFormat_BGRA8UnormSrgb ||
            candidate == WGPUTextureFormat_RGBA8UnormSrgb) {
            state_->surface_format = candidate;
            break;
        }
    }
    state_->alpha_mode = surface_capabilities.alphaModeCount > 0U &&
        surface_capabilities.alphaModes != nullptr
        ? surface_capabilities.alphaModes[0]
        : WGPUCompositeAlphaMode_Auto;
    wgpuSurfaceCapabilitiesFreeMembers(surface_capabilities);

    if (!configure_surface(
            state_->surface, state_->device, state_->surface_format,
            state_->alpha_mode, surface, state_->surface_configuration, error)) {
        return false;
    }

    state_->surface_info = surface;
    capabilities = {};
    capabilities.supports_2d = false;
    capabilities.supports_3d = false;
    capabilities.supports_instancing = false;
    capabilities.maximum_draws_per_frame = 0;
    capabilities.maximum_texture_dimension = 0;

    const std::string ready =
        "wgpu-native surface configured; backend=" +
        std::to_string(static_cast<unsigned>(adapter_info.backendType)) +
        "; extent=" + std::to_string(surface.pixel_extent.width) + "x" +
        std::to_string(surface.pixel_extent.height) +
        "; draw capabilities=none (clear/present milestone)";
    __android_log_write(ANDROID_LOG_INFO, kLogTag, ready.c_str());
    error.clear();
    return true;
}

bool WgpuNativeBackend::resize(
    const RenderSurfaceInfo& surface, std::string& error) {
    if (!state_ || state_->surface == nullptr || state_->device == nullptr) {
        error = "wgpu resize requested before backend initialization";
        return false;
    }
    if (!configure_surface(
            state_->surface, state_->device, state_->surface_format,
            state_->alpha_mode, surface, state_->surface_configuration, error)) {
        return false;
    }
    state_->surface_info = surface;
    return true;
}

bool WgpuNativeBackend::render(
    const RenderFramePacket& frame, std::string& error) {
    if (!state_ || state_->surface == nullptr || state_->device == nullptr ||
        state_->queue == nullptr) {
        error = "wgpu render requested while the backend is not initialized";
        return false;
    }
    if (!frame.draws_2d.empty() || !frame.draws_3d.empty()) {
        error = "wgpu clear/present milestone does not yet support draw commands";
        return false;
    }

    WGPUSurfaceTexture surface_texture{};
    wgpuSurfaceGetCurrentTexture(state_->surface, &surface_texture);
    if (surface_texture.status == WGPUSurfaceGetCurrentTextureStatus_Timeout) {
        error.clear();
        return true;
    }
    if (surface_texture.status == WGPUSurfaceGetCurrentTextureStatus_Outdated) {
        if (surface_texture.texture != nullptr)
            wgpuTextureRelease(surface_texture.texture);
        if (!configure_surface(
                state_->surface, state_->device, state_->surface_format,
                state_->alpha_mode, state_->surface_info,
                state_->surface_configuration, error)) return false;
        return true;
    }
    if (surface_texture.status != WGPUSurfaceGetCurrentTextureStatus_SuccessOptimal &&
        surface_texture.status != WGPUSurfaceGetCurrentTextureStatus_SuccessSuboptimal) {
        if (surface_texture.texture != nullptr)
            wgpuTextureRelease(surface_texture.texture);
        error = "wgpuSurfaceGetCurrentTexture failed (status=" +
            std::to_string(static_cast<unsigned>(surface_texture.status)) + ")";
        return false;
    }
    if (surface_texture.texture == nullptr) {
        error = "wgpuSurfaceGetCurrentTexture succeeded without a texture";
        return false;
    }

    WGPUTextureView frame_view =
        wgpuTextureCreateView(surface_texture.texture, nullptr);
    if (frame_view == nullptr) {
        wgpuTextureRelease(surface_texture.texture);
        error = "wgpuTextureCreateView returned null";
        return false;
    }

    WGPUCommandEncoderDescriptor encoder_descriptor =
        WGPU_COMMAND_ENCODER_DESCRIPTOR_INIT;
    encoder_descriptor.label = {"Dream Engine clear frame encoder", WGPU_STRLEN};
    WGPUCommandEncoder encoder =
        wgpuDeviceCreateCommandEncoder(state_->device, &encoder_descriptor);
    if (encoder == nullptr) {
        wgpuTextureViewRelease(frame_view);
        wgpuTextureRelease(surface_texture.texture);
        error = "wgpuDeviceCreateCommandEncoder returned null";
        return false;
    }

    WGPURenderPassColorAttachment color_attachment =
        WGPU_RENDER_PASS_COLOR_ATTACHMENT_INIT;
    color_attachment.view = frame_view;
    color_attachment.loadOp = frame.clear_color_buffer
        ? WGPULoadOp_Clear : WGPULoadOp_Load;
    color_attachment.storeOp = WGPUStoreOp_Store;
    color_attachment.clearValue = {
        frame.clear_color.r, frame.clear_color.g,
        frame.clear_color.b, frame.clear_color.a
    };

    WGPURenderPassDescriptor pass_descriptor =
        WGPU_RENDER_PASS_DESCRIPTOR_INIT;
    pass_descriptor.colorAttachmentCount = 1;
    pass_descriptor.colorAttachments = &color_attachment;
    WGPURenderPassEncoder pass =
        wgpuCommandEncoderBeginRenderPass(encoder, &pass_descriptor);
    if (pass == nullptr) {
        wgpuCommandEncoderRelease(encoder);
        wgpuTextureViewRelease(frame_view);
        wgpuTextureRelease(surface_texture.texture);
        error = "wgpuCommandEncoderBeginRenderPass returned null";
        return false;
    }
    wgpuRenderPassEncoderEnd(pass);
    wgpuRenderPassEncoderRelease(pass);

    WGPUCommandBufferDescriptor command_descriptor =
        WGPU_COMMAND_BUFFER_DESCRIPTOR_INIT;
    command_descriptor.label = {"Dream Engine clear frame", WGPU_STRLEN};
    WGPUCommandBuffer command =
        wgpuCommandEncoderFinish(encoder, &command_descriptor);
    if (command == nullptr) {
        wgpuCommandEncoderRelease(encoder);
        wgpuTextureViewRelease(frame_view);
        wgpuTextureRelease(surface_texture.texture);
        error = "wgpuCommandEncoderFinish returned null";
        return false;
    }

    wgpuQueueSubmit(state_->queue, 1, &command);
    wgpuSurfacePresent(state_->surface);

    wgpuCommandBufferRelease(command);
    wgpuCommandEncoderRelease(encoder);
    wgpuTextureViewRelease(frame_view);
    wgpuTextureRelease(surface_texture.texture);
    wgpuInstanceProcessEvents(state_->instance);

    error.clear();
    return true;
}

void WgpuNativeBackend::shutdown() noexcept {
    if (!state_) return;
    if (state_->queue != nullptr) {
        wgpuQueueRelease(state_->queue);
        state_->queue = nullptr;
    }
    if (state_->device != nullptr) {
        wgpuDeviceRelease(state_->device);
        state_->device = nullptr;
    }
    if (state_->adapter != nullptr) {
        wgpuAdapterRelease(state_->adapter);
        state_->adapter = nullptr;
    }
    if (state_->surface != nullptr) {
        wgpuSurfaceRelease(state_->surface);
        state_->surface = nullptr;
    }
    if (state_->instance != nullptr) {
        wgpuInstanceRelease(state_->instance);
        state_->instance = nullptr;
    }
    state_->surface_format = WGPUTextureFormat_Undefined;
    state_->alpha_mode = WGPUCompositeAlphaMode_Auto;
    state_->surface_info = {};
}

} // namespace dream::render

#else

namespace dream::render {
// CMake only adds this source for Android builds.
}

#endif
