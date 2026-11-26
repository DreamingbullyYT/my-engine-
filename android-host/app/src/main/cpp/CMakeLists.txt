cmake_minimum_required(VERSION 3.25)
project(DreamEngineAndroidHost LANGUAGES C CXX)

set(DREAM_ENGINE_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../../../..")
set(DREAM_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(DREAM_BUILD_ANDROID_GAME_ACTIVITY_BACKEND ON CACHE BOOL "" FORCE)

add_subdirectory("${DREAM_ENGINE_ROOT}" "${CMAKE_BINARY_DIR}/dream-engine")

add_library(dream_engine_android_host SHARED
    android_main.cpp
)
target_compile_features(dream_engine_android_host PRIVATE cxx_std_20)
target_compile_options(dream_engine_android_host PRIVATE
    -Wall -Wextra -Wpedantic
)
target_link_libraries(dream_engine_android_host PRIVATE
    DreamEngine::PlatformAndroidGameActivity
    android
    log
)
set_target_properties(dream_engine_android_host PROPERTIES
    CXX_EXTENSIONS OFF
    POSITION_INDEPENDENT_CODE ON
)
