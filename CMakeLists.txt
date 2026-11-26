cmake_minimum_required(VERSION 3.25)

project(DreamEngine
    VERSION 0.1.0
    DESCRIPTION "A general-purpose 2D and 3D C++ game engine"
    LANGUAGES C CXX
)

option(DREAM_BUILD_TESTS "Build Dream Engine tests" ON)
option(DREAM_BUILD_BGFX_BACKEND "Build the optional bgfx clear/present backend (requires an existing bgfx target)" OFF)
option(DREAM_BUILD_ANDROID_GAME_ACTIVITY_BACKEND "Build the AndroidX GameActivity platform adapter (requires the GameActivity Prefab package)" OFF)

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

add_library(dream_platform STATIC
    engine/platform/src/application.cpp
    engine/platform/src/filesystem.cpp
)
if(UNIX)
    target_sources(dream_platform PRIVATE engine/platform/src/native_filesystem.cpp)
endif()
add_library(DreamEngine::Platform ALIAS dream_platform)
target_include_directories(dream_platform PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/engine/platform/include>
)
target_compile_features(dream_platform PUBLIC cxx_std_20)
set_target_properties(dream_platform PROPERTIES
    CXX_EXTENSIONS OFF
    POSITION_INDEPENDENT_CODE ON
)

if(DREAM_BUILD_ANDROID_GAME_ACTIVITY_BACKEND)
    if(NOT ANDROID)
        message(FATAL_ERROR "DREAM_BUILD_ANDROID_GAME_ACTIVITY_BACKEND is only available for Android builds")
    endif()
    find_package(game-activity REQUIRED CONFIG)
    add_library(dream_platform_android_game_activity STATIC
        engine/platform/android/src/game_activity_backend.cpp
        engine/platform/android/src/android_asset_filesystem.cpp
    )
    add_library(DreamEngine::PlatformAndroidGameActivity ALIAS dream_platform_android_game_activity)
    target_include_directories(dream_platform_android_game_activity PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/engine/platform/android/include>
    )
    target_compile_features(dream_platform_android_game_activity PUBLIC cxx_std_20)
    target_link_libraries(dream_platform_android_game_activity
        PUBLIC DreamEngine::Platform game-activity::game-activity_static android log
    )
    set_target_properties(dream_platform_android_game_activity PROPERTIES
        CXX_EXTENSIONS OFF
        POSITION_INDEPENDENT_CODE ON
    )
endif()

add_library(dream_dmod STATIC
    engine/dmod/src/manifest.cpp
    engine/dmod/src/manifest_parser.cpp
    engine/dmod/src/package_reader.cpp
    engine/dmod/src/resolver.cpp
    engine/dmod/src/native_library.cpp
    engine/dmod/src/native_mod_host.cpp
    engine/dmod/src/native_hook_registry.cpp
    engine/dmod/src/native_mod_manager.cpp
    engine/dmod/src/native_mod_package_installer.cpp
)
add_library(DreamEngine::DMod ALIAS dream_dmod)

add_library(dream_render INTERFACE)
add_library(DreamEngine::Render ALIAS dream_render)
target_include_directories(dream_render INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/engine/render/include>
)
target_compile_features(dream_render INTERFACE cxx_std_20)
target_link_libraries(dream_render INTERFACE
    DreamEngine::Core DreamEngine::Platform DreamEngine::World
)

if(DREAM_BUILD_BGFX_BACKEND)
    if(NOT TARGET bgfx)
        message(FATAL_ERROR "DREAM_BUILD_BGFX_BACKEND requires a bgfx CMake target")
    endif()

    add_library(dream_render_bgfx STATIC
        engine/render/bgfx/src/bgfx_clear_backend.cpp
    )
    add_library(DreamEngine::RenderBgfx ALIAS dream_render_bgfx)
    target_include_directories(dream_render_bgfx
        PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/engine/render/bgfx/include>
    )
    target_compile_features(dream_render_bgfx PUBLIC cxx_std_20)
    target_link_libraries(dream_render_bgfx
        PUBLIC DreamEngine::Render
        PRIVATE bgfx
    )
    set_target_properties(dream_render_bgfx PROPERTIES
        CXX_EXTENSIONS OFF
        POSITION_INDEPENDENT_CODE ON
    )
endif()

add_library(dream_runtime INTERFACE)
add_library(DreamEngine::Runtime ALIAS dream_runtime)
target_include_directories(dream_runtime INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/engine/runtime/include>
)
target_compile_features(dream_runtime INTERFACE cxx_std_20)
target_link_libraries(dream_runtime INTERFACE
    DreamEngine::Core DreamEngine::Platform DreamEngine::World
    DreamEngine::Input DreamEngine::Physics2D DreamEngine::Render DreamEngine::DMod
)

add_library(dream_physics2d INTERFACE)
add_library(DreamEngine::Physics2D ALIAS dream_physics2d)
target_include_directories(dream_physics2d INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/engine/physics2d/include>
)
target_compile_features(dream_physics2d INTERFACE cxx_std_20)
target_link_libraries(dream_physics2d INTERFACE DreamEngine::Core)

add_library(dream_physics3d INTERFACE)
add_library(DreamEngine::Physics3D ALIAS dream_physics3d)
target_include_directories(dream_physics3d INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/engine/physics3d/include>
)
target_compile_features(dream_physics3d INTERFACE cxx_std_20)
target_link_libraries(dream_physics3d INTERFACE DreamEngine::Core)

add_library(dream_input INTERFACE)
add_library(DreamEngine::Input ALIAS dream_input)
target_include_directories(dream_input INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/engine/input/include>
)
target_compile_features(dream_input INTERFACE cxx_std_20)
target_link_libraries(dream_input INTERFACE DreamEngine::Platform)

add_library(dream_serialization INTERFACE)
add_library(DreamEngine::Serialization ALIAS dream_serialization)
target_include_directories(dream_serialization INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/engine/serialization/include>
)
target_compile_features(dream_serialization INTERFACE cxx_std_20)
target_link_libraries(dream_serialization INTERFACE DreamEngine::Core)

add_library(dream_world INTERFACE)
add_library(DreamEngine::World ALIAS dream_world)
target_include_directories(dream_world INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/engine/world/include>
)
target_compile_features(dream_world INTERFACE cxx_std_20)
target_link_libraries(dream_world INTERFACE DreamEngine::Core DreamEngine::Serialization)

add_library(dream_assets INTERFACE)
add_library(DreamEngine::Assets ALIAS dream_assets)
target_include_directories(dream_assets INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/engine/assets/include>
)
target_compile_features(dream_assets INTERFACE cxx_std_20)
target_link_libraries(dream_assets INTERFACE DreamEngine::Core DreamEngine::Platform)
target_link_libraries(dream_dmod
    PUBLIC DreamEngine::Core
    PRIVATE DreamEngine::Miniz DreamEngine::TomlPlusPlus ${CMAKE_DL_LIBS}
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

    add_executable(dream_core_job_system_tests tests/core/job_system_tests.cpp)
    target_link_libraries(dream_core_job_system_tests PRIVATE DreamEngine::Core)
    target_compile_features(dream_core_job_system_tests PRIVATE cxx_std_20)
    set_target_properties(dream_core_job_system_tests PROPERTIES CXX_EXTENSIONS OFF)
    add_test(NAME dream.core.jobs COMMAND dream_core_job_system_tests)

    add_executable(dream_core_handle_tests tests/core/handle_tests.cpp)
    target_link_libraries(dream_core_handle_tests PRIVATE DreamEngine::Core)
    target_compile_features(dream_core_handle_tests PRIVATE cxx_std_20)
    set_target_properties(dream_core_handle_tests PROPERTIES CXX_EXTENSIONS OFF)
    add_test(NAME dream.core.handles COMMAND dream_core_handle_tests)

    add_executable(dream_core_resource_registry_tests tests/core/resource_registry_tests.cpp)
    target_link_libraries(dream_core_resource_registry_tests PRIVATE DreamEngine::Core)
    target_compile_features(dream_core_resource_registry_tests PRIVATE cxx_std_20)
    set_target_properties(dream_core_resource_registry_tests PROPERTIES CXX_EXTENSIONS OFF)
    add_test(NAME dream.core.resource_registry COMMAND dream_core_resource_registry_tests)

    add_executable(dream_core_resource_store_tests tests/core/resource_store_tests.cpp)
    target_link_libraries(dream_core_resource_store_tests PRIVATE DreamEngine::Core)
    target_compile_features(dream_core_resource_store_tests PRIVATE cxx_std_20)
    set_target_properties(dream_core_resource_store_tests PROPERTIES CXX_EXTENSIONS OFF)
    add_test(NAME dream.core.resource_store COMMAND dream_core_resource_store_tests)

    add_executable(dream_assets_loader_tests tests/assets/asset_loader_tests.cpp)
    target_link_libraries(dream_assets_loader_tests PRIVATE DreamEngine::Assets)
    target_compile_features(dream_assets_loader_tests PRIVATE cxx_std_20)
    set_target_properties(dream_assets_loader_tests PROPERTIES CXX_EXTENSIONS OFF)
    add_test(NAME dream.assets.loader COMMAND dream_assets_loader_tests)

    add_executable(dream_assets_load_scope_tests tests/assets/asset_load_scope_tests.cpp)
    target_link_libraries(dream_assets_load_scope_tests PRIVATE DreamEngine::Assets)
    target_compile_features(dream_assets_load_scope_tests PRIVATE cxx_std_20)
    set_target_properties(dream_assets_load_scope_tests PROPERTIES CXX_EXTENSIONS OFF)
    add_test(NAME dream.assets.load_scope COMMAND dream_assets_load_scope_tests)

    add_executable(dream_assets_request_coordinator_tests tests/assets/asset_request_coordinator_tests.cpp)
    target_link_libraries(dream_assets_request_coordinator_tests PRIVATE DreamEngine::Assets)
    target_compile_features(dream_assets_request_coordinator_tests PRIVATE cxx_std_20)
    set_target_properties(dream_assets_request_coordinator_tests PROPERTIES CXX_EXTENSIONS OFF)
    add_test(NAME dream.assets.request_coordinator COMMAND dream_assets_request_coordinator_tests)

    add_executable(dream_assets_cache_tests tests/assets/asset_cache_tests.cpp)
    target_link_libraries(dream_assets_cache_tests PRIVATE DreamEngine::Assets)
    target_compile_features(dream_assets_cache_tests PRIVATE cxx_std_20)
    set_target_properties(dream_assets_cache_tests PROPERTIES CXX_EXTENSIONS OFF)
    add_test(NAME dream.assets.cache COMMAND dream_assets_cache_tests)

    add_executable(dream_assets_dependency_graph_tests tests/assets/dependency_graph_tests.cpp)
    target_link_libraries(dream_assets_dependency_graph_tests PRIVATE DreamEngine::Assets)
    target_compile_features(dream_assets_dependency_graph_tests PRIVATE cxx_std_20)
    set_target_properties(dream_assets_dependency_graph_tests PROPERTIES CXX_EXTENSIONS OFF)
    add_test(NAME dream.assets.dependency_graph COMMAND dream_assets_dependency_graph_tests)

    add_executable(dream_runtime_tests tests/runtime/engine_runtime_tests.cpp)
    target_link_libraries(dream_runtime_tests PRIVATE DreamEngine::Runtime)
    target_compile_features(dream_runtime_tests PRIVATE cxx_std_20)
    set_target_properties(dream_runtime_tests PROPERTIES CXX_EXTENSIONS OFF)
    add_test(NAME dream.runtime.integration COMMAND dream_runtime_tests)

    add_executable(dream_bgfx_shader_assets_tests tests/render/bgfx_shader_assets_tests.cpp)
    target_link_libraries(dream_bgfx_shader_assets_tests PRIVATE DreamEngine::Render)
    target_include_directories(dream_bgfx_shader_assets_tests PRIVATE engine/render/bgfx/include)
    target_compile_features(dream_bgfx_shader_assets_tests PRIVATE cxx_std_20)
    set_target_properties(dream_bgfx_shader_assets_tests PROPERTIES CXX_EXTENSIONS OFF)
    add_test(NAME dream.render.bgfx_shader_assets COMMAND dream_bgfx_shader_assets_tests)

    add_executable(dream_render_frame_builder_tests tests/render/render_frame_builder_tests.cpp)
    target_link_libraries(dream_render_frame_builder_tests PRIVATE DreamEngine::Render)
    target_compile_features(dream_render_frame_builder_tests PRIVATE cxx_std_20)
    set_target_properties(dream_render_frame_builder_tests PROPERTIES CXX_EXTENSIONS OFF)
    add_test(NAME dream.render.frame_builder COMMAND dream_render_frame_builder_tests)

    add_executable(dream_render_contract_tests tests/render/render_contract_tests.cpp)
    target_link_libraries(dream_render_contract_tests PRIVATE DreamEngine::Render)
    target_compile_features(dream_render_contract_tests PRIVATE cxx_std_20)
    set_target_properties(dream_render_contract_tests PROPERTIES CXX_EXTENSIONS OFF)
    add_test(NAME dream.render.contracts COMMAND dream_render_contract_tests)

    add_executable(dream_physics3d_broadphase_tests tests/physics3d/broadphase_tests.cpp)
    target_link_libraries(dream_physics3d_broadphase_tests PRIVATE DreamEngine::Physics3D)
    target_compile_features(dream_physics3d_broadphase_tests PRIVATE cxx_std_20)
    set_target_properties(dream_physics3d_broadphase_tests PROPERTIES CXX_EXTENSIONS OFF)
    add_test(NAME dream.physics3d.broadphase COMMAND dream_physics3d_broadphase_tests)

    add_executable(dream_physics3d_world_tests tests/physics3d/physics_world_tests.cpp)
    target_link_libraries(dream_physics3d_world_tests PRIVATE DreamEngine::Physics3D)
    target_compile_features(dream_physics3d_world_tests PRIVATE cxx_std_20)
    set_target_properties(dream_physics3d_world_tests PROPERTIES CXX_EXTENSIONS OFF)
    add_test(NAME dream.physics3d.world COMMAND dream_physics3d_world_tests)

    add_executable(dream_physics3d_geometry_tests tests/physics3d/geometry_tests.cpp)
    target_link_libraries(dream_physics3d_geometry_tests PRIVATE DreamEngine::Physics3D)
    target_compile_features(dream_physics3d_geometry_tests PRIVATE cxx_std_20)
    set_target_properties(dream_physics3d_geometry_tests PROPERTIES CXX_EXTENSIONS OFF)
    add_test(NAME dream.physics3d.geometry COMMAND dream_physics3d_geometry_tests)

    add_executable(dream_physics2d_world_tests tests/physics2d/physics_world_tests.cpp)
    target_link_libraries(dream_physics2d_world_tests PRIVATE DreamEngine::Physics2D)
    target_compile_features(dream_physics2d_world_tests PRIVATE cxx_std_20)
    set_target_properties(dream_physics2d_world_tests PROPERTIES CXX_EXTENSIONS OFF)
    add_test(NAME dream.physics2d.world COMMAND dream_physics2d_world_tests)

    add_executable(dream_physics2d_broadphase_tests tests/physics2d/broadphase_tests.cpp)
    target_link_libraries(dream_physics2d_broadphase_tests PRIVATE DreamEngine::Physics2D)
    target_compile_features(dream_physics2d_broadphase_tests PRIVATE cxx_std_20)
    set_target_properties(dream_physics2d_broadphase_tests PROPERTIES CXX_EXTENSIONS OFF)
    add_test(NAME dream.physics2d.broadphase COMMAND dream_physics2d_broadphase_tests)

    add_executable(dream_physics2d_geometry_tests tests/physics2d/geometry_tests.cpp)
    target_link_libraries(dream_physics2d_geometry_tests PRIVATE DreamEngine::Physics2D)
    target_compile_features(dream_physics2d_geometry_tests PRIVATE cxx_std_20)
    set_target_properties(dream_physics2d_geometry_tests PROPERTIES CXX_EXTENSIONS OFF)
    add_test(NAME dream.physics2d.geometry COMMAND dream_physics2d_geometry_tests)

    add_executable(dream_input_action_map_tests tests/input/action_map_tests.cpp)
    target_link_libraries(dream_input_action_map_tests PRIVATE DreamEngine::Input)
    target_compile_features(dream_input_action_map_tests PRIVATE cxx_std_20)
    set_target_properties(dream_input_action_map_tests PROPERTIES CXX_EXTENSIONS OFF)
    add_test(NAME dream.input.action_map COMMAND dream_input_action_map_tests)

    add_executable(dream_scene_archive_tests tests/world/scene_archive_tests.cpp)
    target_link_libraries(dream_scene_archive_tests PRIVATE DreamEngine::World)
    target_compile_features(dream_scene_archive_tests PRIVATE cxx_std_20)
    set_target_properties(dream_scene_archive_tests PROPERTIES CXX_EXTENSIONS OFF)
    add_test(NAME dream.world.scene_archive COMMAND dream_scene_archive_tests)

    add_executable(dream_core_fixed_step_tests tests/core/fixed_step_clock_tests.cpp)
    target_link_libraries(dream_core_fixed_step_tests PRIVATE DreamEngine::Core)
    target_compile_features(dream_core_fixed_step_tests PRIVATE cxx_std_20)
    set_target_properties(dream_core_fixed_step_tests PROPERTIES CXX_EXTENSIONS OFF)
    add_test(NAME dream.core.fixed_step_clock COMMAND dream_core_fixed_step_tests)

    add_executable(dream_world_camera_tests tests/world/camera_tests.cpp)
    target_link_libraries(dream_world_camera_tests PRIVATE DreamEngine::World)
    target_compile_features(dream_world_camera_tests PRIVATE cxx_std_20)
    set_target_properties(dream_world_camera_tests PROPERTIES CXX_EXTENSIONS OFF)
    add_test(NAME dream.world.camera COMMAND dream_world_camera_tests)

    add_executable(dream_world_archive_tests tests/world/world_archive_tests.cpp)
    target_link_libraries(dream_world_archive_tests PRIVATE DreamEngine::World)
    target_compile_features(dream_world_archive_tests PRIVATE cxx_std_20)
    set_target_properties(dream_world_archive_tests PROPERTIES CXX_EXTENSIONS OFF)
    add_test(NAME dream.world.archive COMMAND dream_world_archive_tests)

    add_executable(dream_serialization_tests tests/serialization/binary_archive_tests.cpp)
    target_link_libraries(dream_serialization_tests PRIVATE DreamEngine::Serialization)
    target_compile_features(dream_serialization_tests PRIVATE cxx_std_20)
    set_target_properties(dream_serialization_tests PROPERTIES CXX_EXTENSIONS OFF)
    add_test(NAME dream.serialization.binary_archive COMMAND dream_serialization_tests)

    add_executable(dream_world_scheduler_tests tests/world/system_scheduler_tests.cpp)
    target_link_libraries(dream_world_scheduler_tests PRIVATE DreamEngine::World)
    target_compile_features(dream_world_scheduler_tests PRIVATE cxx_std_20)
    set_target_properties(dream_world_scheduler_tests PROPERTIES CXX_EXTENSIONS OFF)
    add_test(NAME dream.world.scheduler COMMAND dream_world_scheduler_tests)

    add_executable(dream_world_tests tests/world/world_tests.cpp)
    target_link_libraries(dream_world_tests PRIVATE DreamEngine::World)
    target_compile_features(dream_world_tests PRIVATE cxx_std_20)
    set_target_properties(dream_world_tests PROPERTIES CXX_EXTENSIONS OFF)
    add_test(NAME dream.world.ecs COMMAND dream_world_tests)

    add_executable(dream_platform_application_tests tests/platform/application_tests.cpp)
    target_link_libraries(dream_platform_application_tests PRIVATE DreamEngine::Platform)
    target_compile_features(dream_platform_application_tests PRIVATE cxx_std_20)
    set_target_properties(dream_platform_application_tests PROPERTIES CXX_EXTENSIONS OFF)
    add_test(NAME dream.platform.application COMMAND dream_platform_application_tests)

    add_executable(dream_platform_contract_tests tests/platform/platform_contract_tests.cpp)
    target_link_libraries(dream_platform_contract_tests PRIVATE DreamEngine::Platform)
    target_compile_features(dream_platform_contract_tests PRIVATE cxx_std_20)
    set_target_properties(dream_platform_contract_tests PROPERTIES CXX_EXTENSIONS OFF)
    add_test(NAME dream.platform.contracts COMMAND dream_platform_contract_tests)

    if(UNIX)
        add_executable(dream_platform_native_filesystem_tests
            tests/platform/native_filesystem_tests.cpp
        )
        target_link_libraries(dream_platform_native_filesystem_tests
            PRIVATE DreamEngine::Platform
        )
        target_compile_features(dream_platform_native_filesystem_tests PRIVATE cxx_std_20)
        set_target_properties(dream_platform_native_filesystem_tests PROPERTIES CXX_EXTENSIONS OFF)
        add_test(NAME dream.platform.native_filesystem COMMAND dream_platform_native_filesystem_tests)
    endif()

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

    add_executable(dream_dmod_package_reader_tests tests/dmod/package_reader_tests.cpp)
    target_link_libraries(dream_dmod_package_reader_tests
        PRIVATE DreamEngine::DMod DreamEngine::Miniz
    )
    target_compile_features(dream_dmod_package_reader_tests PRIVATE cxx_std_20)
    set_target_properties(dream_dmod_package_reader_tests PROPERTIES CXX_EXTENSIONS OFF)
    add_test(NAME dream.dmod.package_reader COMMAND dream_dmod_package_reader_tests)

    add_library(dream_dmod_test_native_plugin SHARED tests/dmod/native_mod_test_plugin.cpp)
    target_link_libraries(dream_dmod_test_native_plugin PRIVATE DreamEngine::DMod)
    target_compile_features(dream_dmod_test_native_plugin PRIVATE cxx_std_20)
    set_target_properties(dream_dmod_test_native_plugin PROPERTIES
        CXX_EXTENSIONS OFF POSITION_INDEPENDENT_CODE ON
    )

    add_executable(dream_dmod_native_mod_runtime_tests tests/dmod/native_mod_runtime_tests.cpp)
    target_link_libraries(dream_dmod_native_mod_runtime_tests PRIVATE
        DreamEngine::DMod DreamEngine::Miniz
    )
    target_compile_features(dream_dmod_native_mod_runtime_tests PRIVATE cxx_std_20)
    target_compile_definitions(dream_dmod_native_mod_runtime_tests PRIVATE
        DREAM_TEST_NATIVE_MOD_PATH="$<TARGET_FILE:dream_dmod_test_native_plugin>"
    )
    add_dependencies(dream_dmod_native_mod_runtime_tests dream_dmod_test_native_plugin)
    set_target_properties(dream_dmod_native_mod_runtime_tests PROPERTIES CXX_EXTENSIONS OFF)
    add_test(NAME dream.dmod.native_runtime COMMAND dream_dmod_native_mod_runtime_tests)
endif()
