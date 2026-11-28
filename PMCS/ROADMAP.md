# Roadmap

All milestones are PLANNED until evidence is recorded in CURRENT_STATE.md.
## Release scope — D-020

The first playable v1 is a coherent local-system game slice: one starting planet, its Moon, nearby asteroids, a basic functional orbital station, the colony/economy/research loop, a first spacecraft, and basic orbital flight. The v1 exit gate requires these systems to build, save/load, and pass automated tests, plus a user-confirmed Android ARM64 device test.

Interstellar travel, additional star systems, multi-system autonomous fleets/colonization, and full-scale mobile-colony megaships are v2+ work. They remain in the long-term design; do not make them prerequisites for v1. Keep M1 (official Godot Android baseline) and M2 (separate GDExtension proof) as distinct early gates. M1 must not require a custom Godot source build; M2 proves the separate C++ extension can load without modifying Godot.

## First playable vertical slice — immediate priority (approved 2026-11-28)

The first implementation slice is deliberately smaller than all of v1. It must form a coherent playable loop on one starting planet and remain lightweight on Android.

- [x] Main scene opens the playable colony screen; M1–M4 diagnostics remain reachable from that screen.
- [x] One circular planet presentation with surface building markers.
- [x] Five functional building types: habitat, solar array, farm, mine, and launch pad.
- [x] Start with 20 persistent citizen records containing identity, job, health, and happiness.
- [x] Basic housing, worker assignment, food consumption/production, metal production, and energy balance.
- [x] Build a rocket from colony resources, launch with simplified radial thrust and gravity, insert into a circular orbit, simulate orbital motion, and return to the surface.
- [x] Save/load colony, citizens, buildings, resources, and rocket state to one local JSON slot.
- [x] Pass the automated gameplay smoke test (Actions run 46136155425, commit 51e2fc43bacba6432c8d841a0e28e7ec426cac2b).
- [ ] Complete Android ARM64 APK export for the playable slice.
- [ ] User confirms the APK runs and the full loop works on a physical Android device.

Do not expand this slice into advanced needs, detailed citizen routines, excavation, underground colonies, research trees, Moon/asteroid gameplay, interstellar travel, or multiplayer before the core loop is stable. Those remain planned features.

### Art and placeholder budget

The user creates the final game textures/sprites. Every final sprite or texture must be no larger than **32×32 pixels**. Assistant-created temporary visuals must be extremely simple and obey the same maximum; prefer primitive pixel shapes and flat colours, avoid detailed texture work, and keep placeholders easy to replace. Do not generate or add elaborate art assets without a new request.


## M0 — Clean baseline and PMCS
- [x] Empty source branches intentionally on 2026-11-27.
- [x] Resolve scripting-language decision: GDScript is primary for .dmod (D-010).
- [ ] Review and finalize PMCS, including GAMEPLAY.md, ROCKETS_AND_PHYSICS.md, UNIVERSE.md, AUTOMATION.md, SPACE_INFRASTRUCTURE.md, SPACE_EXPANSION.md, PERFORMANCE_AND_SIMULATION.md, and MOBILE_UX.md.
- [x] Confirm repository name and main-only development policy.
Exit: clean baseline and reviewed project memory.

## M1 — Official Godot Android baseline
- [x] Set up the official, unmodified Godot 4.7.2-stable editor and official Android export templates/toolchain in GitHub Actions (verified in run #5); no custom Godot fork.
- [x] Produce a minimal installable APK from the standard Android export pipeline (run #5; 28,431,521 bytes; Android arm64-v8a library and package ID validated). No Godot source compilation was needed.
- [ ] User to validate on physical Android device: launch, visible rendering, touch button/counter, pause/resume, and readable UI.
Exit: user confirms physical-device test.

## M2 — GDExtension proof
Status: implementation merged to main; CI compile/export passed; device validation pending.
- [x] Add an independent, minimal C++ GDExtension module with separate entry, registration, and probe-class files.
- [x] Pin official godot-cpp and target the Godot 4.7 API without modifying Godot source.
- [x] Add a GDScript smoke test for native class registration, string return, and arithmetic call.
- [x] Add a visible boot-screen status for the native probe.
- [x] Cross-compile Linux x86_64 and Android arm64-v8a.
- [x] Export and validate an Android ARM64 APK containing the extension (Actions run #12).
- [ ] User to confirm that the Android APK loads the extension on the physical phone.
Exit: CI build/export and physical device confirms Android loading. CI success alone does not close the device gate.

## M3 — GDScript runtime-loading proof
Status: initial loader and on-device diagnostic UI implemented on main; CI compile/export passed; device validation pending.
- [x] Add an app-private `user://mods/` runtime loader.
- [x] Write/read a sample GDScript from app-private storage, compile it at runtime, instantiate it, and call its entry point.
- [x] Add visible success/failure reporting and source-path logging.
- [x] Export the updated Android ARM64 APK successfully (Actions run #18; artifact ID 15228256470).
- [ ] User to execute the M3 test on physical Android and confirm the success status/log.
- [ ] Follow-up: import an independently supplied script file through Android's system file picker; this is not yet implemented.
Exit: an external GDScript stored outside the APK executes on device and calls the host runtime.

## M4 — DMod manager
Status: initial package manager and diagnostic UI implemented on main; Android ARM64 APK export verified (artifact ID 15230941586); physical-device validation pending.
- [x] Define and validate mod.json schema and .dmod ZIP format.
- [x] Inspect/extract package contents; reject path traversal and enforce configured archive/entry limits (decompression-bomb hardening remains incomplete).
- [x] Resolve exact-version dependencies, detect dependency cycles, reject duplicate archive paths, and load dependencies deterministically.
- [x] Install/enable/disable a sample script mod without APK rebuild (device execution pending).
Exit: sample `.dmod` installed and executed on device; external package import and robust hostile-archive handling must be proven before calling the manager production-ready.

## M5 — Core API
- [ ] Lifecycle, event bus, extensible registries, hooks, direct game integration, mod-owned save/config data.
- [ ] Mod-to-mod public services/events and script reload semantics.
- [ ] Publish stable API IDs/signatures and compatibility policy.
Exit: integration tests and sample mods exercise every public contract.

## M6 — New game systems
- [ ] Enemy/AI sample.
- [ ] Inventory/crafting sample.
- [ ] Vehicle/physics sample.
- [ ] Demonstrate a system absent from the original host game.
- [ ] Validate game-host integration contracts with examples for colony governance (advice → graduated delegation), NPC pilots, modular orbital stations, and mobile colony habitats.
- [ ] Validate mobile-colony fleet planning/construction and permission-gated autonomous colonization after roughly 5–6 explored star systems, including habitability and expedition-supply checks.
- [ ] Ensure host APIs allow modded station/habitat modules and do not hard-code only the base game's content.
Exit: samples work without Godot source modifications or APK rebuild.

## M7 — Multiplayer
- [ ] Define session/authority contract.
- [ ] Define save compatibility and deterministic simulation contract for modded content.
- [ ] Mod-defined messages and schema validation.
- [ ] Sync a mod-owned entity between two clients.
- [ ] Test mismatch, disconnect, malformed packets, authority violations.
Exit: two real clients connect and synchronize the sample mod.

## M8 — Production hardening
- [ ] Profiling, diagnostics, compatibility matrix, modder SDK.
- [ ] Establish measured Android ARM64 budgets for citizens, buildings, vehicles, world bodies, memory, and frame/simulation time.
- [ ] Security and Android policy review.
- [ ] Release and migration policy.

Do not jump to multiplayer or advanced hooks before M1-M4 are proven. Prefer small vertical slices that compile, run, and can be tested.

## Development start authorization — 2026-11-27
The project is approved to move from design/planning into implementation. This records authorization to begin; it does **not** claim that implementation or device validation has already happened.

Execution began with M1 and continued through the M2–M4 implementation proofs. On 2026-11-28 the user explicitly authorized starting the first playable colony-to-orbit slice before physical-device validation of M1–M4 was complete. This authorization changes the implementation order only: it does not waive any CI or physical-device validation gate. The first-slice scope and 32×32 maximum art budget are defined above. Continue directly on main in small, testable increments; GitHub Actions performs repeatable builds/tests and the user performs physical-device validation after an APK is available.

Do not start by implementing the entire game at once. The v1 scope and milestone exit criteria above remain authoritative.
