# Godot Migration — Dream Engine

**Status:** Android bootstrap validated on a physical device; full engine integration in progress
**Updated:** 2026-11-27
**Target:** Godot 4.7.2-stable, Android arm64-v8a, C++ native modules

## Accepted foundation and source pin

The user approved using the real Godot source as the new Dream Engine foundation.

- Upstream: https://github.com/godotengine/godot
- Tag: `4.7.2-stable`
- Exact upstream commit: `ed1daf0bf001b61586d9930840f2f1394092c079`
- The commit is verified as the signed upstream 4.7.2-stable release commit.
- **Required repository layout:** all tracked Godot source files are committed directly under `third_party/godot/`. A git submodule is explicitly not acceptable.
- `third_party/GODOT_SOURCE_COMMIT` records the upstream SHA. CI checks the marker, the source version and that `third_party/godot` is not a gitlink.
- Preserve upstream `LICENSE`, `COPYRIGHT.txt`, `AUTHORS.md` and third-party notices when present. Keep the project's own third-party notices current.

The one-time `.github/workflows/vendor-godot-source.yml` workflow converts the current gitlink to normal tracked files at the exact pinned commit. It also starts the Android build workflow after the vendor commit.

## Active engine architecture

Godot owns the mature platform and general-purpose engine systems: scene tree, resources, rendering, physics, input, audio, animation, UI and Android platform/runtime. Dream Engine-specific behavior is implemented as C++ modules and minimal patches inside the vendored source. Game-specific content remains in Godot projects and resources. C++ is the primary language for engine internals and native mods; GDScript is available for project/game scripting.

The existing standalone C++ engine, GameActivity host, wgpu/bgfx experiments and tests remain in the repository as legacy/reference material until replacements have evidence and are intentionally retired.

## Deep native mod requirement

The old DMod requirement remains mandatory:

- Native mods are prebuilt shared libraries loaded at runtime; no compilation on the user's phone and no APK rebuild for installation.
- Native code executes in-process without a sandbox or capability enforcement. It has the application's process privileges and can crash or corrupt the engine. Only trusted packages should be installed.
- GDExtension remains useful for its public supported interface but is not treated as unrestricted private-engine access.
- The active implementation direction is a built-in Dream Engine C++ Godot module with an append-only versioned native ABI, direct access to live engine objects, owner-tracked native hooks and explicit `Before`, `Around`, `Replace`, and `After` stages.
- Hook points must be integrated into real engine lifecycle/scene/physics/render/resource paths. A registry without engine call sites is not considered deep-hook completion.
- Module unload must deactivate hooks and call mod shutdown before unloading its shared library. Mods are responsible for stopping their own threads and releasing owned resources before shutdown returns.

## Android bootstrap milestone

Workflow: `.github/workflows/godot-android-source.yml`

The workflow compiles the Android debug template from the pinned Godot source with SCons, imports/runs the validation project using the matching official editor archive, exports an ARM64 APK, verifies APK integrity/signature, and byte-compares the packaged `libgodot_android.so` against the source-built library.

**Physical-device smoke test reported by the user on 2026-11-27:** the app displayed “DREAM ENGINE / Powered by Godot 4.7.2”, touch input responded, the presses counter reached 25, and the FPS counter showed 59 FPS. This proves app startup, visible rendering and basic touch delivery on that device. It does not yet prove full renderer, physics, lifecycle, audio or native-mod behavior.

## Remaining foundation work

1. Vendor the full Godot source directly into this repository at the SHA above and remove the `.gitmodules` entry.
2. Build the first in-tree Dream Engine C++ module and connect real main-loop/physics/render/scene/resource hook sites.
3. Integrate the native mod loader, package/manifest/dependency validation, raw engine interfaces, ownership and unload lifecycle.
4. Build and install fresh Android ARM64 APKs from the vendored/modified source.
5. Validate background/resume, rotation, touch, GPU output, audio and crash/log behavior on the phone.
6. Extend and harden engine systems by relying on Godot's existing 2D/3D renderer, physics, animation, UI, assets and audio rather than duplicating them.
7. Add full editor/tooling and validate with different game workloads.

CI compilation is not a substitute for phone testing. Do not mark a feature complete without build/test evidence.
