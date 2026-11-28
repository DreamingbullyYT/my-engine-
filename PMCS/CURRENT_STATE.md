# Current State

Snapshot: 2026-11-28

## Current implementation priority — D-024
- The user changed the implementation sequence: finish the city/colony management phase, with broad feature coverage described in PMCS/CITY_PHASE.md, before implementing playable space systems.
- D-020's local-system v1 destination remains accepted; this is a sequencing decision, not a deletion of space scope.
- During the city phase the game must stay in the city/local surface view. Existing rocket/flight code is dormant and must not be exposed as playable UI or trigger the planet reveal.
- The eventual city-to-space experience remains one persistent world/session under D-023 and modular under D-022.
- City completion is not claimed until every item in the City Completion Gate has been implemented, tested, and accepted by the user.
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
| Colony domain foundation | INITIAL CITY PROTOTYPE; CITY-ONLY SMOKE/DEVICE REVALIDATION PENDING | colony/colony_simulation.gd; PMCS/modules/colony.md |
| City-first continuous world view (D-023/D-024) | SIDE-ON CITY VIEW (D-025) IN SOURCE; LOCAL HEADLESS TESTS PASS; CI/DEVICE VALIDATION PENDING | rendering/world_view.gd; rendering/city/; ui/screens/game_screen.gd; tests/gameplay_smoke.gd; tests/city_view_smoke.gd |
| Building footprints and tap-to-place construction | SOURCE IMPLEMENTED; LOCAL HEADLESS TESTS PASS; CI/DEVICE VALIDATION PENDING | colony/placement.gd; colony/colony_simulation.gd (`can_place`, `build_at`); tests/city_view_smoke.gd |
| Shared starting-body definition | SOURCE IMPLEMENTED; CI PENDING | world/home_world_definition.gd; consumed by renderer and rocket physics |
| Nearest additional celestial target and exploration return rewards | NOT IMPLEMENTED | No authored celestial-body catalogue or exploration service exists yet; do not invent decorative target bodies or free rewards |

## City presentation slice — 2026-11-28 (D-025)
- Replaced the curved-ground placeholder view with a side-on pixel city: sky with day/night, hills, clouds, ground strata, code-drawn buildings, walking citizens, and a rebuilt HUD (resource bar, build cards, tap-to-place, info panel, menu).
- New modules: `colony/placement.gd`, `rendering/city/` (pixel painter, building art, scenery, citizen walkers), `ui/game_theme.gd`, `ui/widgets/`.
- Fixed a GDScript parse error in `ui/screens/boot_screen.gd` (tab/space mix in `_on_close_pressed`).
- Mouse events that Android emulates from touch are now ignored by the world view to avoid double panning.
- Evidence (local, Godot 4.7.2-stable headless on Linux, not CI): `tests/gameplay_smoke.gd` passes; `tests/city_view_smoke.gd` passes 49 checks, including placement overlap rules, tap-to-build through the real main scene, citizen day/night behavior and draw passes. `tests/native_extension_smoke.gd` was not meaningful locally because the native library is only built in CI.
- Screenshots were rendered with Godot on a software OpenGL driver in a virtual display. This is NOT a phone: performance, touch feel and notch/safe-area behavior are unverified.
- Not done: demolition, construction time, a tutorial, real art, sound, and any space content.

## Latest CI failure — 2026-11-28
- M1 run 46139676246 on source commit d1b710ac67edb9df6102167ca533dd190e425786 failed before native compilation/APK export.
- Godot 4.7.2 reported GDScript parse/type-inference errors in rendering/world_view.gd at lines 203 (new_distance) and 360 (window_center). The smoke script then could not load its dependent world-view script and timed out; no APK artifact was uploaded.
- M2 run 46139676241 on the same source commit completed successfully. This confirms the native-extension workflow only; it does not produce the M1 APK.

## Next milestones
1. Fix the two confirmed world_view.gd type annotations and make the city-only smoke test fail promptly on script-load errors.
2. Hide/deactivate playable space controls and launch-pad progression during the city phase; retain only clean module boundaries for later integration.
3. Implement the city feature vertical slices in the dependency order in PMCS/CITY_PHASE.md, with focused tests and PMCS updates after each slice.
4. Pass the City Completion Gate with robust save/migration, performance, CI, and user-confirmed physical-device validation.
5. Only after user acceptance of the city phase, implement playable space systems and activate D-023 camera scale reveal.
6. Separately complete pending M2/M3/M4 device checks; CI status is not device evidence.

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


## D-024 city-only alignment — source patch pending CI
- The playable city screen no longer exposes the launch pad, rocket construction, launch, or return controls. Space/vehicle source is retained but dormant.
- Colony build definitions are limited to city buildings (habitat, solar, farm, mine); fuel and rocket capability are removed from the active colony simulation.
- The city camera remains locked to its close local view; no gameplay action in the city screen triggers the D-023 planet reveal during this phase.
- The M1 smoke test is focused on city-only start, housing, staffing, construction, production, no active launchpad, city camera lock, and colony snapshot restoration.
- Fixed the two confirmed GDScript type-inference errors in rendering/world_view.gd by explicitly typing new_distance as float and window_center as Vector2.
- Save schema 2 now stores colony state only; schema 1 saves remain readable while legacy dormant rocket payload is ignored.
- This change does not implement the full City Completion Gate. Citizen needs/routines, services, advanced buildings, logistics, research, policies/events, mayor, underground construction, analytics, tutorial, and robust saves remain future city-phase work.


## City phase C2 — citizen profiles and essential needs (source commit pending CI)
- Added a dedicated `CitizenModel` domain helper under `colony/citizens/citizen_model.gd`; balance values live in `colony/data/citizen_balance.json`.
- Citizens now have persistent age, home, job/workplace, skill levels, work history, trait list, food/water/rest/housing/social/recreation/safety needs, health, happiness, status, and activity fields.
- Happiness is computed from weighted citizen needs and health, with an inspectable factor/contribution breakdown. The weights and daily need rates are data-driven JSON, not renderer/UI assumptions.
- Added a water pump as a staffed, power-limited producer and a water stock/consumption path. Food/water shortages now reduce individual satisfaction; prolonged critical shortages damage health.
- Added citizen home allocation across habitat capacity, job-to-building links, small skill progress per worked day, and schema-1 citizen normalization.
- Colony snapshot schema is now 2. Schema-1 colony snapshots remain migratable; old launch-pad entities and fuel are discarded, and a starter water reserve is created when missing.
- City smoke coverage now includes citizens, essential needs, water production/shortages, skills/workplace links, happiness breakdown, citizen inspection, snapshot restore, legacy migration, and city camera lock.
- This remains an early city slice. Social/recreation services, complete routines/movement, medical/sanitation/education/safety buildings, population growth, advanced production/logistics, research, policies/events, mayor, underground construction, and complete city UX remain incomplete.

- Static scope audit after the C2 source commit identified repeated local names in the second home-allocation loop; the names were made unique before accepting the city build. CI validation remains pending.

- The first C2 smoke run passed all citizen/needs/migration assertions, then failed only during cleanup because the test called `free()` on a `RefCounted` citizen model. RefCounted lifetime is automatic; the explicit free call was removed. Corrected CI is pending.
