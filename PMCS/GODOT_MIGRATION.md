# Godot Migration — Dream Engine

**Status:** ACCEPTED DIRECTION; source-build integration in progress
**Updated:** 2026-11-27
**Target:** Godot 4.7.2-stable, Android arm64-v8a, GitHub Actions

## Direction override

The user explicitly approved adopting the upstream Godot source as the new Dream Engine foundation. This supersedes the earlier direction to continue building every foundational engine subsystem independently in `engine/`.

Pin:
- Repository: https://github.com/godotengine/godot
- Tag: `4.7.2-stable`
- Exact commit: `ed1daf0bf001b61586d9930840f2f1394092c079`
- License: MIT. Preserve the upstream LICENSE and every third-party notice in the source tree and distributions.

Do not delete the existing `engine/`, `android-host/`, tests or historical CI during migration. Keep them available as source/reference until replacements are built and verified.

## Active architecture

Godot owns the scene tree, resources, rendering, physics, input, audio and Android platform/runtime where its facilities meet requirements. Dream-specific engine internals belong in small C++ modules/patches; game content belongs in Godot projects/resources. C++ remains the default language for engine extensions and native modules; GDScript may be used for project/game scripting.

The old GameActivity/wgpu-native host is legacy/reference code on this branch, not the new Godot host. `godot-project/` is only a validation target (visible UI, FPS readout and touch-input response), not the final game or finished engine product.

## Native mod requirement

The former DMod requirement remains: trusted native mods execute in-process, without a sandbox, and can affect engine/game state. GDExtension is suitable for supported public extension points but must not be presented as arbitrary access to every private Godot internal. Evaluate a small maintained Godot module/patch layer for deep internal hooks, replace/around behavior and stable hook contexts. No native compilation should be required on the user's phone. Native mods are fully trusted code and may corrupt state or crash the process.

## Android ARM64 source-build milestone

Workflow: `.github/workflows/godot-android-source.yml`

1. Checkout the exact Godot submodule commit, including upstream nested submodules.
2. Install JDK 17, Android SDK 36, Build Tools 36.1.0 and Godot 4.7.2-required NDK 29.0.14206865.
3. Compile Godot's Android `template_debug` for `arm64` from source with SCons and generate the Android template/AAR with the upstream Gradle project.
4. Use the matching Godot 4.7.2 Linux editor release only for project import/export; verify its archive SHA-256.
5. Export the bootstrap project to an installable ARM64 debug APK.
6. Compare the packaged `libgodot_android.so` byte-for-byte with the library compiled from the pinned source and upload the APK/templates as a GitHub Actions artifact.

The official Linux editor zip SHA-256 used by the workflow is `cadd3204e728a35d3f13adb7fd0d7902636b79f6b95c40c265eb73b6c35329e4`. The Android runtime template itself is built from the pinned source.

CI proves compilation and APK packaging, not physical GPU correctness. Phone testing (launch, visible first frame, touch, background/resume, rotation and logs) remains open until the user installs the artifact.

## Next stages

- Verify and fix the source-build workflow before merging the migration branch.
- Install and test the bootstrap APK on the user's Galaxy S23 Ultra.
- Define the Dream-specific module/patch boundary and implement the deep native mod/hook API.
- Port game-facing systems incrementally; do not mechanically duplicate Godot's built-in systems.
- Retire old standalone engine components only after validated replacements exist.
- Add a repeatable desktop/editor source-build workflow after Android source template verification.

Until the new workflow completes successfully, this migration is `IN_PROGRESS`, not VERIFIED.
