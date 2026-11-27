# Dream Runtime — native extension

This folder contains the first independent C++ GDExtension proof for Dream Engine.

## Boundary

- Godot stays on the official, unmodified 4.7.2 release.
- The extension is built separately with the official `godot-cpp` bindings.
- The current native class is only `DreamRuntimeProbe`; it verifies registration, GDScript-to-C++ calls, and lifecycle logging.
- Game systems do not belong in this probe. Future native functionality should be split into cohesive modules with explicit ownership and contracts.
- The Linux x86_64 binary exists for repeatable CI runtime tests. Android arm64-v8a is the primary device target.

## Reproducible CI build

From the repository root, with SCons and the Android NDK installed:

```bash
.github/scripts/build_dream_runtime.sh linux x86_64
.github/scripts/build_dream_runtime.sh android arm64
```

The helper checks out the official `godot-cpp 10.0.0-stable` commit `507ed9d840c01a3c5b2a39af8bb4000bfac30bf5`. The binding generator targets Godot API 4.7 and the project's single-precision official engine build.

Generated dependencies, intermediates, and shared libraries are ignored by Git. Do not commit generated native binaries.

## Tests

`tests/native_extension_smoke.gd` runs under the official Godot headless editor/runtime on Linux. It checks that the class is registered, calls the native string and arithmetic methods, and fails the process if the contract is broken. The Android workflow also compiles the arm64 library and exports an APK for physical-device validation.

## Current limits

A successful Linux smoke test plus Android cross-compilation is not proof that Android loaded the library on a physical phone. M2 remains open until the user installs the ARM64 APK and confirms the visible native status, while CI logs confirm extension initialization and shutdown on the Linux runtime.
