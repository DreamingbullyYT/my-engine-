# Roadmap

All milestones are PLANNED until evidence is recorded in CURRENT_STATE.md.

## M0 — Clean baseline and PMCS
- [x] Empty source branches intentionally on 2026-11-27.
- [x] Resolve scripting-language decision: GDScript is primary for .dmod (D-010).
- [ ] Review and finalize PMCS.
- [ ] Confirm repository name and branch strategy.
Exit: clean baseline and reviewed project memory.

## M1 — Official Godot Android baseline
- [ ] Build official unmodified Godot for Android ARM64 in GitHub Actions.
- [ ] Produce a minimal installable APK.
- [ ] Validate launch, visible rendering, touch, pause/resume on device.
Exit: user confirms physical-device test.

## M2 — GDExtension proof
- [ ] Build an independent minimal C++ GDExtension.
- [ ] Load it in Android ARM64 build.
- [ ] Call a native method from a Godot script.
- [ ] Verify initialization/shutdown logs.
Exit: repeatable on-device load test without Godot source edits.

## M3 — GDScript runtime-loading proof
- [ ] Verify dynamic GDScript source loading/compilation using official Godot APIs.
- [ ] Execute a test script from app-private storage on Android ARM64.
- [ ] Verify script-to-game API calls and source-aware error reporting.
- [ ] Confirm selected exported-build settings permit this workflow.
Exit: a GDScript script loaded from outside the APK executes on device.

## M4 — DMod manager
- [ ] Define and validate mod.json schema and .dmod ZIP format.
- [ ] Safely inspect/extract package contents; reject path traversal and oversized entries.
- [ ] Resolve dependencies, versions, conflicts, duplicate IDs, and deterministic load order.
- [ ] Install/enable/disable a script mod without APK rebuild.
Exit: sample .dmod installed and executed on device.

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
- [ ] Mod-defined messages and schema validation.
- [ ] Sync a mod-owned entity between two clients.
- [ ] Test mismatch, disconnect, malformed packets, authority violations.
Exit: two real clients connect and synchronize the sample mod.

## M8 — Production hardening
- [ ] Profiling, diagnostics, compatibility matrix, modder SDK.
- [ ] Security and Android policy review.
- [ ] Release and migration policy.

Do not jump to multiplayer or advanced hooks before M1-M4 are proven. Prefer small vertical slices that compile, run, and can be tested.
