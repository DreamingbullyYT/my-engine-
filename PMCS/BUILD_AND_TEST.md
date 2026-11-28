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
- CI compiles both native targets and exports the Android APK; the export pipeline is the authoritative project resource/script parse validation. A separate `--headless --editor --import --quit` step was removed after Godot 4.7.2 aborted with exit 134 after completing import/editor initialization, while the parallel APK export from the same source commit succeeded. Do not reintroduce this redundant editor-launch step unless a reproducible engine/editor fix is found. The workflow now runs a separate headless gameplay-domain smoke test, but it still does not launch the interactive game or verify Android runtime loading; those checks remain on the physical device.
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
- The user's physical Android test remains distinct from CI and must still be recorded separately. The M1 workflow no longer runs the native runtime smoke script or `--quit-after` game launch.


## First playable gameplay slice

- tests/gameplay_smoke.gd is the focused headless smoke test for starting population, starter buildings, mine production, food supply, rocket construction/launch, orbit insertion, and surface return.
- .github/workflows/android-m1.yml runs this smoke test with the official Godot executable before building the native libraries and exporting the Android ARM64 APK.
- A successful smoke test and APK export verify the tested domain path and package creation only. They do not prove on-device rendering, touch usability, lifecycle behavior, save persistence on Android, or real performance; the user must validate those on the phone.
- CI does not run the game as an interactive visual session and must never be reported as a physical-device test.


### Gameplay smoke-test evidence — 2026-11-28

- The isolated headless smoke test passed in Actions run 46136155425 on commit 51e2fc43bacba6432c8d841a0e28e7ec426cac2b: https://github.com/DreamingbullyYT/my-engine-/actions/runs/46136155425
- The passing checks cover the 20-citizen start, starter buildings, mine construction/metal production, food supply, housing capacity, rocket construction and launch, orbit insertion, positive orbital period, return to surface, and colony/rocket snapshot restoration.
- The smoke test keeps domain Nodes outside the live SceneTree so gameplay timers cannot hold the headless process open on failure. The workflow command is bounded by a 120-second timeout.
- Android ARM64 native compilation/export and physical-device validation are tracked separately and are not implied by this smoke-test pass.


## City-first continuous world camera — D-023

The updated `tests/gameplay_smoke.gd` must check:
- New colony starts with the planetary zoom locked and the complete planet invisible.
- The launch-pad construction event enables rocket-launch capability.
- The camera's initial zoom is city-close and the automatic reveal targets a zoom small enough to fit the complete home planet in the viewport.
- City recenter restores the city-scale target zoom.
- Rocket ascent activates camera following; landing after flight recentres the camera on the same city.
- World rendering and rocket physics read the same normalized home-world radius/gravitational parameter definition.
- Existing colony economy, save snapshot, rocket ascent/orbit/return tests continue to pass.

These headless checks verify state transitions and domain wiring only. They do not prove the visual transition looks correct, touch gesture behavior on the phone, frame pacing, or actual system-map targeting. Physical Android testing remains mandatory. The nearest additional celestial target test cannot be added until an authored body catalogue exists; do not mock a fake planet merely to pass a test.


## City C2 smoke test — pending CI on the current source patch
The city smoke test now checks the citizen profile contract, age/home/job/workplace/skills/traits/activity fields, individual food/water/rest/housing/social/recreation/safety values, data-driven happiness breakdown, staffed water-pump output, food/water shortage effects on health, citizen inspection, schema-2 snapshot restoration, and schema-1 migration. It also confirms that launchpads remain unavailable and the camera stays city-locked. These checks are a headless domain gate only; they do not prove mobile touch/rendering or the full City Completion Gate.
