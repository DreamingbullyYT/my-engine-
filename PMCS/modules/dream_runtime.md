# Dream Runtime native module

Status: initial M2 probe implementation; CI and Android device validation pending.
Source: `native/dream_runtime/`
Milestone: M2 in `PMCS/ROADMAP.md`

## Purpose and ownership

Dream Runtime is the separate C++ GDExtension boundary for native integration and future measured performance-critical facilities. It does not own colony, world, vehicle, save, or UI state. Ordinary gameplay and ordinary `.dmod` code remain GDScript-first. Godot itself remains the official, unmodified upstream build.

## Current entry points

- `src/entry.cpp`: exported `dream_runtime_library_init` GDExtension entry symbol and lifecycle registration.
- `src/register_types.cpp`: registers the native class at scene initialization and prints initialization/shutdown evidence.
- `src/dream_runtime.cpp`: implementation of the deliberately small probe class.
- `src/dream_runtime.h`: typed class declaration.
- `bin/dream_runtime.gdextension`: platform library mapping and Godot 4.7 compatibility floor.

## Current native contract

The only class is `DreamRuntimeProbe : Node`:
- `get_probe_message()` returns `dream-runtime-gdextension-ok`.
- `add_numbers(left, right)` returns the sum.

These methods are a smoke-test seam, not a production game API. Do not add general gameplay or an all-purpose native manager here.

## Build and tests

Run from the repository root in an environment with SCons and the Android NDK installed:

```bash
bash .github/scripts/build_dream_runtime.sh linux x86_64
bash .github/scripts/build_dream_runtime.sh android arm64
```

The helper verifies the official `godot-cpp 10.0.0-stable` commit `507ed9d840c01a3c5b2a39af8bb4000bfac30bf5`. The SCons project targets Godot API 4.7, single precision, and debug templates.

`tests/native_extension_smoke.gd` loads the extension under headless Godot, instantiates the class, checks its return string, and checks `20 + 22 = 42`. The Android boot screen displays the same status when the APK loads and calls the extension.

## Dependencies and compatibility

- Upstream Godot 4.7.2 official editor/export templates.
- Upstream godot-cpp 10.0.0-stable at the immutable commit above.
- SCons; Android NDK 29.0.14206865 for Android CI.
- Initial configured targets: Linux x86_64 and Android arm64-v8a; single precision; debug template.
- Hot reload is disabled in the initial descriptor.

## Invariants and limits

- No Godot source modifications or custom engine fork.
- Native code remains in a separate module with explicit, small responsibilities.
- Do not duplicate GDScript gameplay rules in C++ without profiling and a clear reason.
- C++ extensions are trusted in-process native code; they can crash or corrupt the application.
- Linux headless success plus Android cross-compilation is not proof of Android runtime loading.
- Do not mark M2 complete until both CI gates and the user's physical Android validation are recorded.

## Next work

1. Fix any build/export/runtime errors found by the first M2 Actions run.
2. Verify the Linux smoke test and Android ARM64 APK artifact.
3. User confirms the Android screen reports `Native C++: OK · 20 + 22 = 42`.
4. Only then proceed to M3 external GDScript runtime loading.

Last verified source commit: pending M2 CI.
