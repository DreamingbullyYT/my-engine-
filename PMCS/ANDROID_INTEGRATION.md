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


## Current integrated Android GameActivity host — 2026-11-26

This section supersedes the older note that the engine repository did not contain an APK/Gradle project and the older statement that its APK was only a platform shell.

- The Gradle application lives in `android-host/`, extends AndroidX GameActivity 4.4.2, and targets package `dev.dreamengine.host`; minSdk 26, compile/target SDK 36, NDK 28.2.13676358, Gradle 9.6, CMake 3.31.6, ABI `arm64-v8a`.
- The Java Activity only loads `dream_engine_android_host`; the native `android_main` owns the native loop and creates the platform backend, `EngineRuntime`, bgfx backend, APK AssetManager filesystem, AAudio output and demo scene.
- The APK contains four generated ESSL 300 shader binaries (sprite vertex/fragment and mesh vertex/fragment). The workflow compiles them from pinned bgfx sources before Gradle assembly and verifies the native ARM64 library plus shader assets inside the APK.
- The demo submits 2D sprites and a 3D mesh/material, advances Physics2D and Physics3D, and includes a retained touch button that applies an impulse to the 2D player.
- App pause stops AAudio and prevents simulation/UI updates; resume restarts audio when the renderer is ready. Surface loss shuts down the render device; a recreated surface reinitializes bgfx and uploads GPU resources again.
- UI layout uses GameActivity logical pixels; renderer camera projections use physical framebuffer pixels.
- Logs use Android Logcat tag `DreamEngine` plus `logs/engine.log` under GameActivity `externalDataPath`, falling back to `internalDataPath`. File logs rotate at 8 MiB and retain three backups.

### Latest verification
- Engine CI [45885498968](https://github.com/DreamingbullyYT/my-engine-/actions/runs/45885498968) passed 5/5 jobs on commit `96dc09d43a8b4f244088a9469458d77c3e5bb483`; GCC, Clang and ASan/UBSan passed 47/47 tests each, Android ARM64 and DMod AST proof passed.
- Fresh Android APK build [45885499085](https://github.com/DreamingbullyYT/my-engine-/actions/runs/45885499085) is building that same commit. The artifact is not available until the final upload step succeeds.
- Physical phone testing is reserved for the user. Cross-compilation and APK packaging do not prove that the GLES renderer, touch mapping, AAudio output or lifecycle work on the target device.


## Implementation update — 2026-11-26

This update is authoritative over older checklist/status text above where they conflict.

- **Physics broadphase queries:** 2D and 3D raycast/overlap queries use deterministic AABB candidate queries backed by cached X-interval indices. Updating/removing a proxy invalidates the index. Tests include 96 distant bodies and stable candidate order.
- **Continuous collision detection (limited opt-in):** `BodyDescription2D::bullet` enables analytic swept-circle time-of-impact against static circles; `BodyDescription3D::bullet` enables swept-sphere time-of-impact against static spheres. Other shapes and dynamic-vs-dynamic pairs still use discrete collision detection. Do not describe this as general CCD.
- **OBJ/MTL:** OBJ material-library parsing and concave polygon ear-clipping are implemented. Triangulation uses Newell normal projection and preserves face-corner UV/normal references; tests cover both polygon windings and signed area.
- **Asset lifecycle hooks:** asynchronous loader observers report read/decode/register and cache phases. The runtime queues owned events with sequence numbers and dispatches native hooks from the engine update thread, not worker threads. Queue capacity is bounded; overflow is logged.
- **Native world hooks:** ECS entity/component/hierarchy mutation hooks run outside the World lock, with the original operation continuation carried in the typed mutation context.
- **Android shader reproducibility:** the four ESSL 300 binaries in the APK are checked in with SHA-256. The Android workflow validates them directly; the separate pinned bgfx proof recompiles from source and compares the binaries byte-for-byte.
- **Android host:** GameActivity host, EngineRuntime, bgfx GLES 3 renderer, 2D/3D demo, retained touch UI, basic software-keyboard/IME bridge, AAudio output connection, pause/surface recreation handling and rotating logs are present. CI proves cross-compilation/package layout only, not physical-device rendering/audio/IME behavior.
- **Sleeping:** not yet implemented; no public sleep/wake state is exposed. It remains open rather than being marked complete.
- **Remaining production work:** general CCD for non-circular shapes and dynamic-vs-dynamic pairs; sleeping/waking; joints; multiple fixtures and robust contact manifolds; complete MTL texture handling and glTF/GLB/image/audio import; GPU asset residency/device-loss policy beyond renderer resource reinitialization; full IME composition/selection; full font shaping/atlas and UI clipping; animation blends/events/curves; native mod resource/job/thread ownership and hot reload; package signatures/trust; profiler/editor; two real-game integration tests; physical-device verification.
