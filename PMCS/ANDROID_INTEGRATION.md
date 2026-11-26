# Android GameActivity integration

Dream Engine provides an Android platform adapter for AndroidX GameActivity. The host app owns the Java/Kotlin Activity and the `android_main(android_app*)` entry point; the engine does not define either one.

## Host application dependencies

Use AndroidX GameActivity 4.4.2 and enable Prefab in the Android app module:

```kotlin
android {
    buildFeatures {
        prefab = true
    }
}

dependencies {
    implementation("androidx.games:games-activity:4.4.2")
}
```

The app module should configure its native build with `DREAM_BUILD_ANDROID_GAME_ACTIVITY_BACKEND=ON` and `find_package(game-activity REQUIRED CONFIG)` is performed by the engine's optional target. The target links `game-activity::game-activity_static`, `android` and `log`. GameActivity includes its own `android_native_app_glue`; do not also link the older NDK glue implementation.

## Native entry point

```cpp
#include <dream/platform/android/game_activity_backend.hpp>

extern "C" void android_main(android_app* app) {
    dream::platform::AndroidGameActivityBackend backend(app);
    dream::platform::Application application(backend);
    (void)application.run();
}
```

A real game should create its logical `WindowId` from its initialization callback, acquire/release a surface around renderer initialization, and drain window/input events every frame. Android controls the Activity's actual window dimensions and presentation mode; `WindowConfig` is validated but does not resize or replace the host Activity's `SurfaceView`.

## APK packaged assets
Use `AndroidAssetFileSystem` for read-only files packaged under the app's `assets/` directory. Construct it from the host Activity's `AAssetManager*`; the manager is borrowed and must outlive the filesystem and every active read.

```cpp
#include <dream/platform/android/android_asset_filesystem.hpp>

// From the host Activity/native app glue:
auto* manager = app->activity->assetManager;
dream::platform::AndroidAssetFileSystem packaged_assets(manager);
```

Pass virtual paths relative to APK assets, for example `textures/terrain.png` (do not include the `assets/` prefix). Reads are bounded by `max_bytes`, preserve the output vector on failure, and support compressed or uncompressed APK assets through the NDK asset API. Writes, directory creation and removal return `FileError::Unsupported`. Directory listing exposes regular files only; the NDK asset directory API does not expose complete directory metadata. Use a separate writable `NativeFileSystem` rooted in app-private storage for saves, mods and caches.

## Lifecycle and ownership

- The adapter is attached to the `android_app` supplied to `android_main` and must be used on that native-app-glue thread.
- It temporarily installs an `onAppCmd` callback and clears the default key/motion filters to accept mouse, stylus and gamepad sources. Previous callbacks and filters are restored on shutdown.
- The previous `onAppCmd` callback is chained; `userData` is not overwritten.
- `SurfaceCreated` and `SurfaceDestroyed` events tell the host to initialize or tear down its graphics surface. Drain these events before issuing rendering work for the frame.
- Each acquired `SurfaceId` owns an `ANativeWindow_acquire` reference and must be released. The adapter releases any remaining leases during shutdown.
- Input uses GameActivity's swapped `android_input_buffer` and clears both event arrays after consuming them. The adapter maps Android key codes to USB HID usages where known; unrecognized keys are omitted. It explicitly enables non-default joystick axes and converts pointer coordinates from Android surface pixels into the logical-pixel convention used by the engine. `unicodeChar` is exposed as best-effort `TextInput`; full IME composition and soft-keyboard state are a separate integration task.
- The adapter does not create an APK, manifest, Java/Kotlin Activity or Gradle project. Those remain the responsibility of the embedding application.
## Current APK/host status — 2026-11-26

This update supersedes the older note that the repository did not yet contain a Gradle host or APK workflow.

- Gradle Android application path: `android-host/`.
- Activity extends AndroidX `GameActivity` 4.4.2; namespace/application ID `dev.dreamengine.host`.
- minSdk 26, compile/target SDK 36, NDK `28.2.13676358`, CMake 3.31.6, Gradle 9.6, ABI filter `arm64-v8a`.
- Native host links `DreamEngine::PlatformAndroidGameActivity` and `DreamEngine::AudioAndroidAAudio`; Java loads `dream_engine_android_host`.
- `android_main` creates `AndroidGameActivityBackend`, creates a native window, runs the platform `Application` loop and drains window/input events. Window is destroyed on shutdown.
- APK build workflow verifies that `lib/arm64-v8a/libdream_engine_android_host.so` is present and uploads the installable debug APK. Emulator smoke test was removed; physical testing is reserved for the user.

### Successful artifact
[Android APK run 45859471974](https://github.com/DreamingbullyYT/my-engine-/actions/runs/45859471974) produced artifact `dream-engine-android-host-arm64-debug` (ID `15138770221`, 3,145,963 bytes, SHA-256 `345306a9572ae92f5b88052c4be06ae9835235a8b5ca2dd6a81fbf15614261e6`, expires 2026-12-10).

That APK run used commit `ee57832717d141e8408f9e6887d402a9ef371ca8`. The Activity/native host/CMake files have not changed in the later engine service-hook commit. The current engine runtime itself was separately cross-compiled successfully in [CI run 45859870368](https://github.com/DreamingbullyYT/my-engine-/actions/runs/45859870368), but do not assume that this older APK contains every later engine runtime change.

### Critical current limitation
The APK is presently a native host/window/lifecycle smoke application. It does **not** yet instantiate `EngineRuntime`, initialize bgfx, package/load the generated shader binaries, or draw either sample game. It is not a playable game build.

### Next Android steps
1. Construct EngineRuntime from the GameActivity native lifecycle and map input, pause/resume, surface recreation and frame delta.
2. Package ESSL300 sprite/mesh shaders as Android assets and load them before bgfx initialization.
3. Initialize bgfx against the current native surface; handle orientation/density/resize and surface loss.
4. Render the 2D/3D sample on device, connect AudioMixer to AAudio output, and rebuild/upload APK.
5. User performs installation/launch/resume/pause/rotation test on their phone; record exact model/API results. CI cross-compilation alone never marks this complete.
