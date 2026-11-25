cmake_minimum_required(VERSION 3.25)

project(DreamEngine
    VERSION 0.1.0
    DESCRIPTION "A general-purpose 2D and 3D C++ game engine"
    LANGUAGES CXX
)

option(DREAM_BUILD_TESTS "Build Dream Engine tests" ON)

add_library(dream_core STATIC
    engine/core/src/job_system.cpp
    engine/core/src/log.cpp
    engine/core/src/version.cpp
)

add_library(DreamEngine::Core ALIAS dream_core)

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

    add_executable(dream_core_job_system_tests tests/core/job_system_tests.cpp)
    target_link_libraries(dream_core_job_system_tests PRIVATE DreamEngine::Core)
    target_compile_features(dream_core_job_system_tests PRIVATE cxx_std_20)
    set_target_properties(dream_core_job_system_tests PROPERTIES CXX_EXTENSIONS OFF)
    add_test(NAME dream.core.jobs COMMAND dream_core_job_system_tests)
endif()
