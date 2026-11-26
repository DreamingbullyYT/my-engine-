cmake_minimum_required(VERSION 3.25)
project(DreamEngineAndroidHost LANGUAGES C CXX)

set(DREAM_ENGINE_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../../../..")

# The Android APK contains the same pinned bgfx backend used by the desktop
# proof. CI checks out the exact bgfx/bx/bimg revisions before Gradle runs.
foreach(required_dir IN ITEMS BGFX_CMAKE_SOURCE_DIR BGFX_DIR BX_DIR BIMG_DIR)
    if(DEFINED ENV{${required_dir}} AND NOT "$ENV{${required_dir}}" STREQUAL "")
        set(${required_dir} "$ENV{${required_dir}}" CACHE PATH "" FORCE)
    endif()
    if(NOT DEFINED ${required_dir} OR "${${required_dir}}" STREQUAL "")
        message(FATAL_ERROR "${required_dir} must point to a pinned source checkout")
    endif()
endforeach()

set(BGFX_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
set(BGFX_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BGFX_BUILD_EXAMPLE_COMMON OFF CACHE BOOL "" FORCE)
set(BGFX_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(BGFX_BUILD_TOOLS_BIN2C OFF CACHE BOOL "" FORCE)
set(BGFX_BUILD_TOOLS_SHADER OFF CACHE BOOL "" FORCE)
set(BGFX_BUILD_TOOLS_GEOMETRY OFF CACHE BOOL "" FORCE)
set(BGFX_BUILD_TOOLS_TEXTURE OFF CACHE BOOL "" FORCE)
set(BGFX_INSTALL OFF CACHE BOOL "" FORCE)
set(BGFX_CUSTOM_TARGETS OFF CACHE BOOL "" FORCE)
set(BGFX_WITH_WAYLAND OFF CACHE BOOL "" FORCE)
set(BGFX_CONFIG_VIDEO OFF CACHE BOOL "" FORCE)
set(BIMG_CONFIG_DECODE_ENABLE OFF CACHE BOOL "" FORCE)
set(BIMG_CONFIG_PARSE_ENABLE OFF CACHE BOOL "" FORCE)
set(BIMG_CONFIG_USE_STB_IMAGE OFF CACHE BOOL "" FORCE)

add_subdirectory("${BGFX_CMAKE_SOURCE_DIR}" "${CMAKE_BINARY_DIR}/bgfx-cmake")

set(DREAM_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(DREAM_BUILD_ANDROID_GAME_ACTIVITY_BACKEND ON CACHE BOOL "" FORCE)
set(DREAM_BUILD_ANDROID_AAUDIO_BACKEND ON CACHE BOOL "" FORCE)
set(DREAM_BUILD_BGFX_BACKEND ON CACHE BOOL "" FORCE)

add_subdirectory("${DREAM_ENGINE_ROOT}" "${CMAKE_BINARY_DIR}/dream-engine")

find_library(ANDROID_SYSTEM_LIB android REQUIRED)
find_library(ANDROID_EGL_LIB EGL REQUIRED)
find_library(ANDROID_GLES_LIB GLESv2 REQUIRED)
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
    DreamEngine::RenderBgfx
    ${ANDROID_SYSTEM_LIB}
    ${ANDROID_EGL_LIB}
    ${ANDROID_GLES_LIB}
    ${ANDROID_LOG_LIB}
)
set_target_properties(dream_engine_android_host PROPERTIES
    CXX_EXTENSIONS OFF
    POSITION_INDEPENDENT_CODE ON
)
