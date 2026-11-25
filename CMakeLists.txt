cmake_minimum_required(VERSION 3.25)

project(DreamEngine
    VERSION 0.1.0
    DESCRIPTION "A general-purpose 2D and 3D C++ game engine"
    LANGUAGES C CXX
)

option(DREAM_BUILD_TESTS "Build Dream Engine tests" ON)

include(FetchContent)
include(GenerateExportHeader)
find_package(Threads REQUIRED)

# Dependencies are pinned to immutable upstream commits.
FetchContent_Declare(
    miniz
    GIT_REPOSITORY https://github.com/richgel999/miniz.git
    GIT_TAG 77d0dce8627735138c51770d1799a1ef48f2117d
    GIT_SHALLOW FALSE
)
FetchContent_GetProperties(miniz)
if(NOT miniz_POPULATED)
    FetchContent_Populate(miniz)
endif()

FetchContent_Declare(
    tomlplusplus
    GIT_REPOSITORY https://github.com/marzer/tomlplusplus.git
    GIT_TAG efd80e03356af65c7d4e61c56ffb326b34f8d0f4
    GIT_SHALLOW FALSE
)
FetchContent_GetProperties(tomlplusplus)
if(NOT tomlplusplus_POPULATED)
    FetchContent_Populate(tomlplusplus)
endif()

add_library(dream_miniz STATIC
    ${miniz_SOURCE_DIR}/miniz.c
    ${miniz_SOURCE_DIR}/miniz_zip.c
    ${miniz_SOURCE_DIR}/miniz_tinfl.c
    ${miniz_SOURCE_DIR}/miniz_tdef.c
)
add_library(DreamEngine::Miniz ALIAS dream_miniz)
generate_export_header(dream_miniz
    BASE_NAME miniz
    EXPORT_MACRO_NAME MINIZ_EXPORT
)
target_include_directories(dream_miniz PUBLIC
    $<BUILD_INTERFACE:${miniz_SOURCE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)
target_compile_definitions(dream_miniz
    PUBLIC MINIZ_STATIC_DEFINE
    PRIVATE MINIZ_NO_STDIO
)
set_target_properties(dream_miniz PROPERTIES
    C_STANDARD 90
    C_STANDARD_REQUIRED ON
    C_EXTENSIONS OFF
    POSITION_INDEPENDENT_CODE ON
)

add_library(dream_tomlplusplus INTERFACE)
add_library(DreamEngine::TomlPlusPlus ALIAS dream_tomlplusplus)
target_include_directories(dream_tomlplusplus INTERFACE
    $<BUILD_INTERFACE:${tomlplusplus_SOURCE_DIR}/include>
)
target_compile_features(dream_tomlplusplus INTERFACE cxx_std_17)

add_library(dream_core STATIC
    engine/core/src/job_system.cpp
    engine/core/src/log.cpp
    engine/core/src/version.cpp
)

add_library(DreamEngine::Core ALIAS dream_core)

add_library(dream_dmod STATIC
    engine/dmod/src/manifest.cpp
    engine/dmod/src/manifest_parser.cpp
    engine/dmod/src/resolver.cpp
)
add_library(DreamEngine::DMod ALIAS dream_dmod)
target_link_libraries(dream_dmod
    PUBLIC DreamEngine::Core
    PRIVATE DreamEngine::Miniz DreamEngine::TomlPlusPlus
)
target_include_directories(dream_dmod PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/engine/dmod/include>
)
target_compile_features(dream_dmod PUBLIC cxx_std_20)
set_target_properties(dream_dmod PROPERTIES CXX_EXTENSIONS OFF POSITION_INDEPENDENT_CODE ON)
target_link_libraries(dream_core PUBLIC Threads::Threads)

target_include_directories(dream_core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/engine/core/include>
)

target_compile_features(dream_core PUBLIC cxx_std_20)

set_target_properties(dream_core PROPERTIES
    CXX_EXTENSIONS OFF
    POSITION_INDEPENDENT_CODE ON
)

if(DREAM_BUILD_TESTS AND NOT ANDROID)
    enable_testing()

    add_executable(dream_core_version_tests tests/core/version_tests.cpp)
    target_link_libraries(dream_core_version_tests PRIVATE DreamEngine::Core)
    target_compile_features(dream_core_version_tests PRIVATE cxx_std_20)
    set_target_properties(dream_core_version_tests PROPERTIES CXX_EXTENSIONS OFF)
    add_test(NAME dream.core.version COMMAND dream_core_version_tests)

    add_executable(dream_core_log_tests tests/core/log_tests.cpp)
    target_link_libraries(dream_core_log_tests PRIVATE DreamEngine::Core)
    target_compile_features(dream_core_log_tests PRIVATE cxx_std_20)
    set_target_properties(dream_core_log_tests PROPERTIES CXX_EXTENSIONS OFF)
    add_test(NAME dream.core.logging COMMAND dream_core_log_tests)

    add_executable(dream_core_event_tests tests/core/event_tests.cpp)
    target_link_libraries(dream_core_event_tests PRIVATE DreamEngine::Core)
    target_compile_features(dream_core_event_tests PRIVATE cxx_std_20)
    set_target_properties(dream_core_event_tests PROPERTIES CXX_EXTENSIONS OFF)
    add_test(NAME dream.core.events COMMAND dream_core_event_tests)

    add_executable(dream_dmod_manifest_tests tests/dmod/manifest_tests.cpp)
    target_link_libraries(dream_dmod_manifest_tests PRIVATE DreamEngine::DMod)
    target_compile_features(dream_dmod_manifest_tests PRIVATE cxx_std_20)
    set_target_properties(dream_dmod_manifest_tests PROPERTIES CXX_EXTENSIONS OFF)
    add_test(NAME dream.dmod.manifest COMMAND dream_dmod_manifest_tests)

    add_executable(dream_dmod_manifest_parser_tests tests/dmod/manifest_parser_tests.cpp)
    target_link_libraries(dream_dmod_manifest_parser_tests PRIVATE DreamEngine::DMod)
    target_compile_features(dream_dmod_manifest_parser_tests PRIVATE cxx_std_20)
    set_target_properties(dream_dmod_manifest_parser_tests PROPERTIES CXX_EXTENSIONS OFF)
    add_test(NAME dream.dmod.manifest_parser COMMAND dream_dmod_manifest_parser_tests)

    add_executable(dream_core_job_system_tests tests/core/job_system_tests.cpp)
    target_link_libraries(dream_core_job_system_tests PRIVATE DreamEngine::Core)
    target_compile_features(dream_core_job_system_tests PRIVATE cxx_std_20)
    set_target_properties(dream_core_job_system_tests PROPERTIES CXX_EXTENSIONS OFF)
    add_test(NAME dream.core.jobs COMMAND dream_core_job_system_tests)
endif()
