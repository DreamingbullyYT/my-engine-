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

## Lifecycle and ownership

- The adapter is attached to the `android_app` supplied to `android_main` and must be used on that native-app-glue thread.
- It temporarily installs an `onAppCmd` callback and clears the default key/motion filters to accept mouse, stylus and gamepad sources. Previous callbacks and filters are restored on shutdown.
- The previous `onAppCmd` callback is chained; `userData` is not overwritten.
- `SurfaceCreated` and `SurfaceDestroyed` events tell the host to initialize or tear down its graphics surface. Drain these events before issuing rendering work for the frame.
- Each acquired `SurfaceId` owns an `ANativeWindow_acquire` reference and must be released. The adapter releases any remaining leases during shutdown.
- Input uses GameActivity's swapped `android_input_buffer` and clears both event arrays after consuming them. The adapter maps Android key codes to USB HID usages where known; unrecognized keys are omitted. It explicitly enables non-default joystick axes and converts pointer coordinates from Android surface pixels into the logical-pixel convention used by the engine. `unicodeChar` is exposed as best-effort `TextInput`; full IME composition and soft-keyboard state are a separate integration task.
- The adapter does not create an APK, manifest, Java/Kotlin Activity or Gradle project. Those remain the responsibility of the embedding application.
