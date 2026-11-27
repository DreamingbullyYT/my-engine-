# Roadmap

All milestones are PLANNED until evidence is recorded in CURRENT_STATE.md.

## M0 — Clean baseline and PMCS
- [x] Empty source branches intentionally on 2026-11-27.
- [ ] Review and finalize PMCS.
- [ ] Confirm repository name and branch strategy.
- [ ] Pin exact Godot release/tag and upstream commit.
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

## M3 — Lua proof
- [ ] Select/implement maintained Lua 5.4 integration.
- [ ] Verify ARM64 compatibility and license.
- [ ] Execute a Lua script from app-private storage.
- [ ] Expose one typed Dream API binding and capture source-aware errors.
Exit: Lua executes on device through the GDExtension.

## M4 — DMod manager
- [ ] Define manifest schema and package validation.
- [ ] Safely read .dmod packages.
- [ ] Resolve dependencies, versions, conflicts, and load order.
- [ ] Install/enable/disable/uninstall a script mod without APK rebuild.
Exit: sample mod installed and executed on device.

## M5 — Core API
- [ ] Lifecycle, event bus, registries, capabilities, hooks.
- [ ] Mod-to-mod API and script reload.
Exit: integration tests and sample mods exercise every public contract.

## M6 — New game systems
- [ ] Enemy/AI sample.
- [ ] Inventory/crafting sample.
- [ ] Vehicle/physics sample.
- [ ] Demonstrate a system absent from the original host game.
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
