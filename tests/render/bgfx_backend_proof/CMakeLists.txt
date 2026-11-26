cmake_minimum_required(VERSION 3.21)
project(DreamEngineBgfxCompileProof LANGUAGES C CXX)

foreach(required_dir IN ITEMS BGFX_CMAKE_SOURCE_DIR BGFX_DIR BX_DIR BIMG_DIR)
    if(NOT DEFINED ${required_dir} OR "${${required_dir}}" STREQUAL "")
        message(FATAL_ERROR "${required_dir} must point to a pinned source checkout")
    endif()
endforeach()

# Keep examples and asset-conversion tools disabled. The Linux proof can opt
# into shaderc to produce portable 2D shader binaries; Android only cross-builds
# the backend library and consumes those precompiled assets at runtime.
option(DREAM_BGFX_BUILD_SHADER_TOOLS "Build host shaderc in this proof" OFF)
set(BGFX_BUILD_TOOLS ${DREAM_BGFX_BUILD_SHADER_TOOLS} CACHE BOOL "" FORCE)
set(BGFX_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BGFX_BUILD_EXAMPLE_COMMON OFF CACHE BOOL "" FORCE)
set(BGFX_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(BGFX_BUILD_TOOLS_BIN2C OFF CACHE BOOL "" FORCE)
set(BGFX_BUILD_TOOLS_SHADER ${DREAM_BGFX_BUILD_SHADER_TOOLS} CACHE BOOL "" FORCE)
set(BGFX_BUILD_TOOLS_GEOMETRY OFF CACHE BOOL "" FORCE)
set(BGFX_BUILD_TOOLS_TEXTURE OFF CACHE BOOL "" FORCE)
set(BGFX_INSTALL OFF CACHE BOOL "" FORCE)
set(BGFX_CUSTOM_TARGETS OFF CACHE BOOL "" FORCE)
set(BGFX_WITH_WAYLAND OFF CACHE BOOL "" FORCE)
set(BGFX_CONFIG_VIDEO OFF CACHE BOOL "" FORCE)
set(BIMG_CONFIG_DECODE_ENABLE OFF CACHE BOOL "" FORCE)
set(BIMG_CONFIG_PARSE_ENABLE OFF CACHE BOOL "" FORCE)
set(BIMG_CONFIG_USE_STB_IMAGE OFF CACHE BOOL "" FORCE)

add_subdirectory("${BGFX_CMAKE_SOURCE_DIR}" bgfx-cmake)

add_library(dream_render_bgfx_bootstrap STATIC
    "${CMAKE_CURRENT_LIST_DIR}/../../../engine/render/bgfx/src/bgfx_clear_backend.cpp"
)
target_include_directories(dream_render_bgfx_bootstrap PUBLIC
    "${CMAKE_CURRENT_LIST_DIR}/../../../engine/render/bgfx/include"
    "${CMAKE_CURRENT_LIST_DIR}/../../../engine/render/include"
    "${CMAKE_CURRENT_LIST_DIR}/../../../engine/platform/include"
    "${CMAKE_CURRENT_LIST_DIR}/../../../engine/world/include"
    "${CMAKE_CURRENT_LIST_DIR}/../../../engine/core/include"
)
target_compile_features(dream_render_bgfx_bootstrap PUBLIC cxx_std_20)
target_link_libraries(dream_render_bgfx_bootstrap PUBLIC bgfx)
set_target_properties(dream_render_bgfx_bootstrap PROPERTIES CXX_EXTENSIONS OFF)

add_executable(dream_bgfx_compile_proof main.cpp)
target_compile_features(dream_bgfx_compile_proof PRIVATE cxx_std_20)
target_link_libraries(dream_bgfx_compile_proof PRIVATE dream_render_bgfx_bootstrap)
set_target_properties(dream_bgfx_compile_proof PROPERTIES CXX_EXTENSIONS OFF)

if(ANDROID)
    find_library(ANDROID_SYSTEM_LIB android REQUIRED)
    find_library(ANDROID_EGL_LIB EGL REQUIRED)
    find_library(ANDROID_GLES_LIB GLESv2 REQUIRED)
    find_library(ANDROID_LOG_LIB log REQUIRED)
    target_link_libraries(dream_bgfx_compile_proof PRIVATE
        ${ANDROID_SYSTEM_LIB} ${ANDROID_EGL_LIB} ${ANDROID_GLES_LIB} ${ANDROID_LOG_LIB})
endif()
