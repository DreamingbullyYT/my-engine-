# Build and Test Policy

## Constraints
- User has no PC development environment.
- GitHub Actions is the primary build/test environment.
- Primary Android ABI: arm64-v8a.
- User validates installable APKs on a physical phone.
- CI success is not equivalent to device success.

## Workflow requirements
- Pin action versions and external dependencies.
- Pin Godot release/tag and commit SHA.
- Build from a clean checkout in release/scheduled validation.
- Upload APKs, native libraries, test reports, and logs separately.
- Artifact names include commit SHA and ABI.
- Fail if expected output is missing or zero bytes.
- Never expose signing keys or secrets in logs/artifacts.
- Use least-privilege workflow permissions.
- Download game assets only when explicitly required and legally permitted.

## Test layers
1. Static checks: formatting, lint, manifest/schema validation.
2. Unit tests: package parser, dependency solver, registries, event dispatch, hook ordering.
3. Integration: Godot + GDExtension, runtime GDScript loading, mod lifecycle and API calls.
4. Android build: ARM64 compile/link/package.
5. Device: launch, rendering, touch, lifecycle, .dmod install/execution, networking.

## Native GDExtension validation (M2)
- Native source lives in `native/dream_runtime/`; Godot source remains untouched.
- `.github/scripts/build_dream_runtime.sh` checks out the official `godot-cpp 10.0.0-stable` pin and builds the requested target with SCons.
- CI compiles Linux x86_64 and Android arm64-v8a native libraries, then exports an Android ARM64 debug APK.
- CI deliberately does not execute Godot headless runtime smoke tests; runtime validation is deferred to the user's physical Android device.
- `tests/native_extension_smoke.gd` remains a manual/development diagnostic; it is not run by the build-only CI workflow.
- The Android export must include the Android GDExtension binary and preserve package/ABI validation.
- User device test must confirm the boot-screen native status says `Native C++: OK · 20 + 22 = 42`.
- A successful compile and APK export do not prove the Android loader works; the user validates loading and runtime behavior on the actual device.

## External GDScript runtime validation (M3)
- `mods/mod_runtime.gd` is an autoload and owns the external script diagnostic.
- The diagnostic creates `user://mods/`, writes a sample GDScript to app-private storage, reads it back, compiles it using `GDScript.reload()`, instantiates the script, and invokes its entry method.
- The sample calls back into the host runtime and emits an explicit success signal; parse/file errors are reported with context.
- The boot screen exposes a manual M3 button and visible status. CI only compiles/exports the APK; the user confirms execution on the phone.
- This proves the Godot runtime-loading path when device-tested; it does not yet prove Android file-picker import or arbitrary third-party .dmod packages.
- Mods are trusted code, not sandboxed.

## GDScript mod and DMod validation (M4)
- Verify script source can be loaded/compiled at runtime in the exact exported Godot configuration.
- Test app-private `user://mods/` access and Android file-picker import.
- Test `.dmod` ZIP validation, path traversal rejection, configured resource caps, duplicate paths, dependency ordering/cycles, and error attribution.\n- ZIPReader.read_file() materializes a decompressed entry as a PackedByteArray; size checks performed afterward are not a complete defense against decompression bombs. Production hardening requires central-directory size preflight or a bounded native ZIP reader.
- Confirm mods can be installed and executed without rebuilding the APK. Current M4 proof UI creates a sample `.dmod` internally; Android system file-picker import remains outstanding.
- Verify disabling a mod prevents its entrypoint from executing.
- Do not claim script mods are sandboxed; the design is trusted-code execution.

## Completion evidence
Record commit SHA, workflow run ID/URL, artifact name/size, test results, user-confirmed device outcome, and known limitations. A green compile proves only that the build succeeded; it does not prove runtime correctness, API completeness, performance, or device compatibility.

## Android build trigger coverage
The Android build workflow must run when Godot project code, scenes, resources, native extension sources, tests, native build scripts, or game assets change, including files added under future modular domains such as world, terrain, colony, vehicles, physics, rendering, and infrastructure. It should not run for documentation-only PMCS edits. Keep path filters broad enough to cover new modules without requiring a manual workflow edit for every new gameplay directory.

## Branch and integration policy — 2026-11-27
- The user requires all future project work to happen directly on `main`; do not create feature branches or ordinary PRs.
- Make small commits to `main`, with focused tests and PMCS updates. Inspect latest main before each change.
- The one-time M2 transition PR #1 was merged after all required CI checks passed. Do not bypass protections.
- The user's physical Android test remains distinct from CI and must still be recorded separately.
