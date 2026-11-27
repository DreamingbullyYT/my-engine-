# Current State

Snapshot: 2026-11-27
Default branch: `main`
Transition branch: `m2/gdextension-proof` (one-time existing PR #1 only)
Repository: DreamingbullyYT/my-engine-

## Verified baseline
- Official Godot 4.7.2-stable project; Godot source is not forked or modified.
- M1 CI run #5 succeeded: https://github.com/DreamingbullyYT/my-engine-/actions/runs/46042193393.
- M1 APK: Android arm64-v8a, package `com.dreamingbully.dreamengine`, 28,431,521 bytes; artifact ID 15220028245.
- M1 physical-device launch, visible UI, touch, pause/resume remain pending user confirmation.
- GDScript is the accepted primary language for ordinary `.dmod` mods (D-010). Mods are trusted code, not sandboxed.
- Modular-by-default architecture is accepted (D-022). The current project is still a boot foundation, not the playable game.

## M2 — Dream Runtime GDExtension
- Separate C++ GDExtension in `native/dream_runtime/`; official Godot source remains untouched.
- Official `godot-cpp 10.0.0-stable`, pinned to `507ed9d840c01a3c5b2a39af8bb4000bfac30bf5`.
- SCons build helper compiles Linux x86_64 and Android arm64-v8a.
- Diagnostic GDScript probe checks native class registration, string return, and `20 + 22 = 42`.
- Boot screen reports native loading status.
- M2 Actions run #12 SUCCEEDED: https://github.com/DreamingbullyYT/my-engine-/actions/runs/46052597705.
- Run #12 compiled both native targets, exported and validated the Android ARM64 debug APK, and uploaded artifact ID 15224884512 (29,635,841 bytes).
- CI deliberately does not run Godot headless runtime tests. The APK must still be installed on a physical Android device to prove native loading.
- **M2 status: CI compile/export VERIFIED; Android device load PENDING.** Do not call M2 fully complete until the user confirms the boot screen shows `Native C++: OK · 20 + 22 = 42`.

## Current implementation matrix
| Area | Status | Evidence |
|---|---|---|
| Official Godot project and Android export | CI VERIFIED; DEVICE TEST PENDING | M1 run #5 |
| Dream Runtime C++ GDExtension | COMPILE + APK EXPORT VERIFIED; DEVICE LOAD PENDING | M2 run #12; `native/dream_runtime/` |
| GDScript-to-native diagnostic probe | IMPLEMENTED; DEVICE VALIDATION PENDING | `tests/native_extension_smoke.gd`; boot screen |
| External GDScript runtime loading | NOT IMPLEMENTED/VERIFIED | M3 |
| .dmod package manager/runtime | NOT IMPLEMENTED | M4 |
| Stable public mod API, registries, events and hooks | NOT IMPLEMENTED | M5 |
| Gameplay systems | NOT IMPLEMENTED | v1 scope D-020 and gameplay/physics/infrastructure specifications |

## Next milestones
1. M2 physical-device validation: install the M2 APK and confirm native status.
2. M3: load and execute an external GDScript from app-private storage in the exported Android build, with game-API call and source-aware errors.
3. M4: validate the `.dmod` ZIP format and manifest, safe extraction, dependency resolution, deterministic load order, and install/enable/disable a sample mod without rebuilding the APK.
4. After M2–M4 implementations are ready, perform the planned device tests for M1/M2/M3/M4. Do not start gameplay implementation until these tests are complete and their outcomes recorded.
5. Then begin the actual game in small, modular, tested vertical slices.

## Main-only development policy
The user requires all future implementation, fixes, documentation, tests, and workflow changes to be committed directly to `main`; no feature branches or routine PRs. PR #1 is the one-time M2 transition exception because it was opened before this rule. Merge only after required CI checks pass and conflicts are resolved. Never bypass protections. After this transition, all work stays on `main`.

## Historical/design authority
Earlier source branches were intentionally emptied on 2026-11-27. Historical C++/Lua experiments, packers, and previous engine code are not current implementation and must not be restored without reviewing exact commits, licenses, dependencies, and tests. Accepted decisions D-001 through D-022 and detailed game requirements remain in PMCS. These are design contracts, not claims that gameplay systems already exist.
