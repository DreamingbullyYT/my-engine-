cmake_minimum_required(VERSION 3.25)
project(DreamEngineAndroidHost LANGUAGES C CXX)

set(DREAM_ENGINE_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../../../..")

if(DEFINED ENV{WGPU_NATIVE_ROOT} AND NOT "$ENV{WGPU_NATIVE_ROOT}" STREQUAL "")
    set(WGPU_NATIVE_ROOT "$ENV{WGPU_NATIVE_ROOT}" CACHE PATH "" FORCE)
endif()
if(NOT DEFINED WGPU_NATIVE_ROOT OR "${WGPU_NATIVE_ROOT}" STREQUAL "")
    message(FATAL_ERROR "WGPU_NATIVE_ROOT must point to the verified wgpu-native Android package")
endif()

set(DREAM_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(DREAM_BUILD_ANDROID_GAME_ACTIVITY_BACKEND ON CACHE BOOL "" FORCE)
set(DREAM_BUILD_ANDROID_AAUDIO_BACKEND ON CACHE BOOL "" FORCE)
set(DREAM_BUILD_BGFX_BACKEND OFF CACHE BOOL "" FORCE)
set(DREAM_BUILD_WGPU_NATIVE_BACKEND ON CACHE BOOL "" FORCE)

add_subdirectory("${DREAM_ENGINE_ROOT}" "${CMAKE_BINARY_DIR}/dream-engine")

find_library(ANDROID_SYSTEM_LIB android REQUIRED)
find_library(ANDROID_LOG_LIB log REQUIRED)

add_library(dream_engine_android_host SHARED
    android_main.cpp
)
target_compile_features(dream_engine_android_host PRIVATE cxx_std_20)
target_compile_options(dream_engine_android_host PRIVATE
    -Wall -Wextra -Wpedantic
)
target_link_libraries(dream_engine_android_host PRIVATE
    DreamEngine::Runtime
    DreamEngine::PlatformAndroidGameActivity
    DreamEngine::AudioAndroidAAudio
    DreamEngine::RenderWgpuNative
    ${ANDROID_SYSTEM_LIB}
    ${ANDROID_LOG_LIB}
)
set_target_properties(dream_engine_android_host PROPERTIES
    CXX_EXTENSIONS OFF
    POSITION_INDEPENDENT_CODE ON
)
