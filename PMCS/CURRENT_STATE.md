# Current State

Snapshot: 2026-11-28
Default branch: `main`
Repository: DreamingbullyYT/my-engine-

## Verified baseline
- Official Godot 4.7.2-stable project; Godot source is not forked or modified.
- M1 baseline run #5 succeeded: https://github.com/DreamingbullyYT/my-engine-/actions/runs/46042193393.
- Latest pre-gameplay M1 run #24 succeeded after removing the redundant editor-import process: https://github.com/DreamingbullyYT/my-engine-/actions/runs/46066459602.
- Run #24 Android ARM64 artifact: dream-engine-m2-b7ae7e6b829bd71be60d1b8b2b644a309ca931ba-android-arm64, artifact ID 15230074820, 29,650,481 bytes; expires 2026-12-27.
- M1 physical-device launch, visible UI, touch, pause/resume remain pending user confirmation.
- GDScript is the accepted primary language for ordinary `.dmod` mods (D-010). Mods are trusted code, not sandboxed.
- Modular-by-default architecture is accepted (D-022). The colony/production/rocket domain logic exists, and its original headless smoke test passed on commit 51e2fc43bacba6432c8d841a0e28e7ec426cac2b (Actions run 46136155425). The planet-first circular overview in rendering/planet_view.gd is explicitly REJECTED as the main player view under D-023. The replacement city-first world camera, launch-pad reveal, single-scene diagnostics overlay, and rocket camera-follow/return behavior are now implemented in source on main; the updated CI smoke test and Android export are pending. The nearest additional celestial destination and mission-return rewards are not implemented because no authored body catalogue/exploration service exists yet.
- All work after the one-time M2 transition PR is committed directly to `main`.

## M2 — Dream Runtime GDExtension
- Separate C++ GDExtension in `native/dream_runtime/`; official Godot source remains untouched.
- Official `godot-cpp 10.0.0-stable`, pinned to `507ed9d840c01a3c5b2a39af8bb4000bfac30bf5`.
- SCons build helper compiles Linux x86_64 and Android arm64-v8a.
- Diagnostic GDScript probe checks native class registration, string return, and `20 + 22 = 42`.
- Boot screen reports native loading status.
- M2 Actions run #12 succeeded: https://github.com/DreamingbullyYT/my-engine-/actions/runs/46052597705.
- Run #12 compiled both native targets, exported and validated the Android ARM64 debug APK, and uploaded artifact ID 15224884512 (29,635,841 bytes).
- M2 integration PR #1 was merged into `main` as commit `c79b5d8cb48f70fe845da4e09f870b256d371aff`, after both required PR checks passed.
- CI deliberately does not run Godot headless runtime tests. The APK must still be installed on a physical Android device to prove native loading.
- **M2 status: CI compile/export VERIFIED; Android device load PENDING.** Do not call M2 fully complete until the user confirms the boot screen shows `Native C++: OK · 20 + 22 = 42`.

## M3 — External GDScript runtime loading
- Added `mods/mod_runtime.gd` as a small autoload-owned runtime loader.
- The M3 diagnostic writes a sample GDScript to `user://mods/_m3_external_probe.gd`, reads it back from app-private storage, compiles it with `GDScript.reload()`, instantiates it, and executes its entry method.
- The external script calls back into the host runtime; the UI displays success/failure and the loader logs the source path.
- Added a touch button and status label to run the proof manually.
- M3 Actions run #18 succeeded: https://github.com/DreamingbullyYT/my-engine-/actions/runs/46057843776; Android ARM64 APK artifact ID 15228256470 (29,639,009 bytes; expires 2026-12-27).
- **M3 status: IMPLEMENTED; CI COMPILE/EXPORT VERIFIED; physical-device execution PENDING.** This does not yet prove arbitrary third-party scripts or Android file-picker import.

## M4 — .dmod package manager (initial implementation)
- Added `mods/mod_package_manager.gd` and registered `ModPackageManager` autoload.
- `.dmod` is a ZIP archive with root `mod.json`, a GDScript entry path, API version 1, semantic numeric version, and exact-version dependency declarations.
- Implemented package size/entry-count limits, path traversal and unsafe-path rejection, duplicate-path rejection, manifest validation, staged extraction, and atomic staging-to-install directory rename.
- Implemented dependency loading in sorted order, exact version checks, dependency-cycle detection, script compilation, entry instantiation, enable/disable lifecycle hooks, and visible result reporting.
- Boot screen can generate an internal sample `.dmod`, install it, execute its entrypoint, and disable it. The controls are inside a scrollable mobile panel. This proves the package pipeline without needing a new APK for the mod itself once device-validated.
- Limitations: Android system file-picker import is not implemented yet; the current UI generates a sample archive internally. `ZIPReader.read_file()` decompresses an entire entry before GDScript can inspect its resulting size, so post-read size caps do not fully prevent a malicious ZIP decompression bomb. Do not describe this parser as hardened against hostile packages until pre-decompression size checks or a bounded native ZIP reader are added.
- M4 Android ARM64 build/export passed in M2 workflow run #46065263621 on source commit `0649b81c786945b064293adb1fef64a11aa4665e`: artifact ID `15230941586`, 29,650,499 bytes. The separate M1 workflow on later workflow-only commit `e0a1e6a` compiled both native targets successfully but its redundant `--headless --editor --import --quit` validation process aborted with exit 134 after project import/editor initialization; no GDScript or scene error was printed. The same source commit had already exported successfully in the parallel M2 run. The dedicated editor-import step was removed; Android export itself remains the authoritative project import/parse validation gate.
- **M4 status: INITIAL IMPLEMENTATION COMMITTED; ANDROID ARM64 APK EXPORT VERIFIED; physical-device execution PENDING.**
- M1 run #24 succeeded with the corrected build-only workflow; the earlier exit-134 editor-import issue did not recur after removing the standalone editor-import step.

## Current implementation matrix
| Area | Status | Evidence |
|---|---|---|
| Official Godot project and Android export | CI VERIFIED; DEVICE TEST PENDING | M1 run #5 |
| Dream Runtime C++ GDExtension | COMPILE + APK EXPORT VERIFIED; DEVICE LOAD PENDING | M2 run #12; `native/dream_runtime/` |
| External GDScript runtime loading | IMPLEMENTED; BUILD/DEVICE TEST PENDING | `mods/mod_runtime.gd`; boot-screen M3 control |
| .dmod package manager/runtime | INITIAL IMPLEMENTATION; CI APK EXPORT VERIFIED; DEVICE TEST PENDING | `mods/mod_package_manager.gd`; M4 boot-screen control |
| Stable public mod API, registries, events and hooks | NOT IMPLEMENTED | M5 |
| Colony/production/rocket domain foundation | SOURCE IMPLEMENTED; HEADLESS SMOKE PASS; DEVICE TEST PENDING | colony/colony_simulation.gd, vehicles/rocket_flight.gd, saves/save_service.gd; Actions run 46136155425 |
| City-first continuous world camera (D-023) | SOURCE IMPLEMENTED; UPDATED CI SMOKE + APK EXPORT PENDING; DEVICE VISUAL TEST PENDING | rendering/world_view.gd; ui/screens/game_screen.gd; tests/gameplay_smoke.gd |
| Shared starting-body definition | SOURCE IMPLEMENTED; CI PENDING | world/home_world_definition.gd; consumed by renderer and rocket physics |
| Nearest additional celestial target and exploration return rewards | NOT IMPLEMENTED | No authored celestial-body catalogue or exploration service exists yet; do not invent decorative target bodies or free rewards |

## Next milestones
1. Run the updated gameplay smoke test and Android ARM64 export; fix any GDScript/import/export failures before publishing an APK.
2. Validate on the phone: city-first opening, no full planet before the launch-pad unlock, complete home-planet reveal, pinch/drag, City recenter, rocket follow, return-to-city, diagnostics overlay, and save/load.
3. Add the authored celestial-body catalogue and focus the nearest real planet/Moon/asteroid from that data; until then the camera reveals the home planet only and must not invent a target.
4. Connect local exploration outcomes and resource/science return to the same colony economy. This is still needed for a genuinely complete city↔space gameplay loop.
5. Separately finish M2/M3/M4 device checks. Continue to defer interstellar travel, complex needs, deep excavation, and other later systems until the local continuous loop is stable.

## Main-only development policy
The user requires all future implementation, fixes, documentation, tests, and workflow changes to be committed directly to `main`; no feature branches or routine PRs. PR #1 was the one-time M2 transition exception and is now merged. Never bypass protections. Continue with small, focused commits and inspect latest `main` before each change.

## Historical/design authority
Earlier source branches were intentionally emptied on 2026-11-27. Historical C++/Lua experiments, packers, and previous engine code are not current implementation and must not be restored without reviewing exact commits, licenses, dependencies, and tests. Accepted decisions D-001 through D-023 and detailed game requirements remain in PMCS. These are design contracts, not claims that gameplay systems already exist.


## D-023 presentation correction — 2026-11-28
- User rejected the first planet-first circular view. It must not be treated as the approved main game presentation.
- Accepted direction: one persistent world session and one continuous city-first camera. Opening view is close to the city; the full planet stays hidden until the actual rocket-launch capability unlock, which triggers a smooth camera reveal to the home planet and the nearest real catalogue target.
- This is not a request for a monolithic script. Internal module ownership remains as D-022.
- Current colony/production/rocket domain smoke test remains valid evidence for those domain rules only. It does not validate the rejected screen or the new camera progression.


## D-023 implementation notes — source status pending CI, 2026-11-28
- `app/main.tscn` still instantiates one `GameScreen`. Its `WorldView` is a full-screen presentation control; UI panels and M1–M4 diagnostics are overlays in the same scene.
- `rendering/world_view.gd` starts on the city's local radial surface. The full planet is hidden at start; building the launch pad sets the prototype's real rocket-capability flag and starts the camera reveal. Planet overview zoom is scaled to fit the viewport, rather than clipping the globe.
- During rocket ascent/orbit/return, the view follows the same rocket position. A user camera gesture cancels automatic following; landing recentres the view on the same city. A City button provides manual recentering.
- `world/home_world_definition.gd` is the single source for the prototype's normalized home-world radius and gravitational parameter; the values are simulation units, not real-world measurements. Rendering and rocket physics consume this definition instead of duplicating constants.
- The current world data contains only the starting body definition. There is no additional authored planet/Moon/asteroid catalogue or exploration/reward service yet. The camera therefore reveals the home planet only. The next local-system target must come from real catalogue data; no decorative planet or unearned resource reward is to be fabricated.
- All visuals in this source slice are code-drawn primitive placeholders; no large texture/image assets are introduced. User art remains capped at 32×32 px per image file.
