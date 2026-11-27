# Current State

Snapshot: 2026-11-27
Default branch: `main`
Repository: DreamingbullyYT/my-engine-

## Verified baseline
- Official Godot 4.7.2-stable project; Godot source is not forked or modified.
- M1 CI run #5 succeeded: https://github.com/DreamingbullyYT/my-engine-/actions/runs/46042193393.
- M1 APK: Android arm64-v8a, package `com.dreamingbully.dreamengine`, 28,431,521 bytes; artifact ID 15220028245.
- M1 physical-device launch, visible UI, touch, pause/resume remain pending user confirmation.
- GDScript is the accepted primary language for ordinary `.dmod` mods (D-010). Mods are trusted code, not sandboxed.
- Modular-by-default architecture is accepted (D-022). The current project is still a boot/validation foundation, not the playable game.
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
- **M3 status: IMPLEMENTED; Android APK build and physical-device execution PENDING.** This does not yet prove arbitrary third-party scripts or Android file-picker import.

## Current implementation matrix
| Area | Status | Evidence |
|---|---|---|
| Official Godot project and Android export | CI VERIFIED; DEVICE TEST PENDING | M1 run #5 |
| Dream Runtime C++ GDExtension | COMPILE + APK EXPORT VERIFIED; DEVICE LOAD PENDING | M2 run #12; `native/dream_runtime/` |
| External GDScript runtime loading | IMPLEMENTED; BUILD/DEVICE TEST PENDING | `mods/mod_runtime.gd`; boot-screen M3 control |
| .dmod package manager/runtime | NOT IMPLEMENTED | M4 |
| Stable public mod API, registries, events and hooks | NOT IMPLEMENTED | M5 |
| Gameplay systems | NOT IMPLEMENTED | v1 scope D-020 and gameplay/physics/infrastructure specifications |

## Next milestones
1. M2 physical-device validation: install the M2/M3 APK and confirm native status.
2. M3 validation: press the external-script test button and confirm `External GDScript: OK · loaded from user://mods/`; inspect log for `[M3] EXTERNAL_GDSCRIPT_OK`.
3. M4: validate the `.dmod` ZIP format and manifest, safe extraction, dependency resolution, deterministic load order, and install/enable/disable a sample mod without rebuilding the APK.
4. After M2–M4 implementations are ready, perform the planned device tests for M1/M2/M3/M4. Do not start gameplay implementation until these tests are complete and their outcomes recorded.
5. Then begin the actual game in small, modular, tested vertical slices.

## Main-only development policy
The user requires all future implementation, fixes, documentation, tests, and workflow changes to be committed directly to `main`; no feature branches or routine PRs. PR #1 was the one-time M2 transition exception and is now merged. Never bypass protections. Continue with small, focused commits and inspect latest `main` before each change.

## Historical/design authority
Earlier source branches were intentionally emptied on 2026-11-27. Historical C++/Lua experiments, packers, and previous engine code are not current implementation and must not be restored without reviewing exact commits, licenses, dependencies, and tests. Accepted decisions D-001 through D-022 and detailed game requirements remain in PMCS. These are design contracts, not claims that gameplay systems already exist.
