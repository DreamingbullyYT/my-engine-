# Current State

Snapshot: 2026-11-27
Default branch: `main`
Active implementation branch: `m2/gdextension-proof`
Repository: DreamingbullyYT/my-engine-

## Verified baseline

- The active game project is a minimal, official Godot 4.7.2-stable project; Godot source is not forked or modified.
- M1 CI run #5 succeeded: https://github.com/DreamingbullyYT/my-engine-/actions/runs/46042193393.
- The M1 artifact is an Android arm64-v8a debug APK, package `com.dreamingbully.dreamengine`, 28,431,521 bytes; artifact ID 15220028245.
- M1 physical-device launch, visible UI, touch, and pause/resume are still awaiting user confirmation. CI export does not count as device validation.
- GDScript is the accepted primary language for ordinary `.dmod` mods (D-010). The runtime mod loader/API is not implemented.
- Modular-by-default architecture is accepted (D-022). The current implementation remains a small boot project, not the game itself.

## M2 — implementation in progress

Branch: `m2/gdextension-proof`

Added, not yet CI-verified:
- A separate `native/dream_runtime` C++ GDExtension module with one probe Node, registration/entry-point split across focused source files, and a Godot 4.7 API target.
- Official `godot-cpp 10.0.0-stable` pinned to commit `507ed9d840c01a3c5b2a39af8bb4000bfac30bf5`; no Godot source modifications.
- Reproducible SCons build helper for Linux x86_64 smoke tests and Android arm64-v8a.
- A GDScript smoke test that instantiates the native class, checks a native string, and checks `20 + 22 = 42`.
- Boot-screen status reporting so the Android APK visibly reports whether the native extension loaded.
- Separate M2 workflow and updated Android build workflow. Their first CI result is still pending.

Do not mark M2 verified until Actions proves native initialization, GDScript-to-C++ calls, shutdown, Android arm64 compilation, and APK export. Physical Android loading remains a separate user-device check.

## Current implementation matrix

| Area | Status | Evidence |
|---|---|---|
| Official Godot project and Android export | CI VERIFIED; DEVICE TEST PENDING | M1 run #5 and artifact 15220028245 |
| Boot UI and Android lifecycle host | PARTIAL | `app/`, `core/app_lifecycle.gd`, `ui/screens/` |
| Dream Runtime C++ GDExtension | IMPLEMENTED ON M2 BRANCH; CI PENDING | `native/dream_runtime/` |
| GDScript-to-native smoke test | IMPLEMENTED ON M2 BRANCH; CI PENDING | `tests/native_extension_smoke.gd` |
| Android ARM64 native load | NOT VERIFIED | Requires M2 CI and physical-device validation |
| External GDScript runtime loading | NOT VERIFIED | M3 |
| .dmod manager/runtime and stable game API | NOT IMPLEMENTED | M4–M5 |
| Gameplay systems | NOT IMPLEMENTED | v1 scope in D-020 and GAMEPLAY/physics/infrastructure specs |
| CodeGraphContext native graph | PENDING REFRESH | New C++ source exists on M2 branch; generated map is still from earlier source |

## Next actions

1. Complete M2 CI; fix compile, load, or export failures before expanding the native runtime.
2. Produce an M2 ARM64 APK and have the user confirm the on-device native status.
3. Keep the M1 device test recorded separately; do not infer it from M2.
4. Proceed to M3 only after the external GDScript loading proof is designed and tested.
5. Update PMCS and generated code maps against the exact successful source commit.

## Historical and design authority

Earlier source branches were intentionally emptied on 2026-11-27. Historical C++/Lua experiments, packers, and previous engine code are not current implementation and must not be restored without reviewing exact commits, licenses, and tests.

Accepted design decisions D-001 through D-022 and detailed game requirements remain in `PMCS/DECISIONS.md`, `PMCS/ARCHITECTURE.md`, `PMCS/MODULES.md`, `PMCS/GAMEPLAY.md`, `PMCS/ROCKETS_AND_PHYSICS.md`, `PMCS/UNIVERSE.md`, `PMCS/AUTOMATION.md`, `PMCS/SPACE_INFRASTRUCTURE.md`, `PMCS/SPACE_EXPANSION.md`, and `PMCS/PERFORMANCE_AND_SIMULATION.md`. These are design contracts, not claims that those gameplay systems already exist.
