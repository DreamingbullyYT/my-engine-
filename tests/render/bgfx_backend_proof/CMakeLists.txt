cmake_minimum_required(VERSION 3.21)
project(DreamEngineBgfxCompileProof LANGUAGES C CXX)

foreach(required_dir IN ITEMS BGFX_CMAKE_SOURCE_DIR BGFX_DIR BX_DIR BIMG_DIR)
    if(NOT DEFINED \${required_dir} OR "\${\${required_dir}}" STREQUAL "")
        message(FATAL_ERROR "\${required_dir} must point to a pinned source checkout")
    endif()
endforeach()

# Keep this isolated: compile/link the library only, without its examples,
# shader tools, asset tools or runtime window-system example glue.
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

add_subdirectory("\${BGFX_CMAKE_SOURCE_DIR}" bgfx-cmake)

add_executable(dream_bgfx_compile_proof main.cpp)
target_compile_features(dream_bgfx_compile_proof PRIVATE cxx_std_20)
target_link_libraries(dream_bgfx_compile_proof PRIVATE bgfx)
set_target_properties(dream_bgfx_compile_proof PROPERTIES CXX_EXTENSIONS OFF)

if(ANDROID)
    find_library(ANDROID_SYSTEM_LIB android REQUIRED)
    find_library(ANDROID_EGL_LIB EGL REQUIRED)
    find_library(ANDROID_GLES_LIB GLESv2 REQUIRED)
    find_library(ANDROID_LOG_LIB log REQUIRED)
    target_link_libraries(dream_bgfx_compile_proof PRIVATE
        \${ANDROID_SYSTEM_LIB} \${ANDROID_EGL_LIB} \${ANDROID_GLES_LIB} \${ANDROID_LOG_LIB})
endif()
