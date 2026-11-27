# Current State

Snapshot: 2026-11-27
Default branch: main
Repository: DreamingbullyYT/my-engine-

## Verified
- Repository exists and is public.
- main and godot-4.7.2-migration were moved to empty-tree commits on 2026-11-27.
- The earlier source branches were intentionally emptied on 2026-11-27. A new minimal M1 Godot project and its first Android host code have since been added to main.
- Earlier pre-reset engine/gameplay implementation is still not restored; only the new minimal M1 files listed below are current source.
- The accepted mod scripting direction is GDScript-first .dmod, recorded in D-010. This is a documentation decision, not an implementation.

## Historical work — NOT current implementation
Earlier project work reportedly included:
- Godot 4.7.2 source vendoring at upstream commit ed1daf0bf001b61586d9930840f2f1394092c079.
- Android ARM64 source-build workflow and a successful build artifact.
- A basic Android test showing Godot version, FPS, and touch-press count.
- Dream-specific C++/Lua experiments, a DMod packer, and package tests.

These are historical facts, not claims that the code still exists or works now. Earlier Lua experiments do not change the accepted GDScript-first design. Recover only deliberately after reviewing exact historical commit, license, dependencies, and tests.

## Current implementation matrix
| Area | Status | Evidence |
|---|---|---|
| PMCS and GDScript decision | DOCUMENTED | Current PMCS and D-010 |
| Game-specific GDScript mod proposal | DOCUMENTED | Root MOD_API.md |
| Godot project integration | PARTIAL | project.godot, app/main.tscn, core/app_lifecycle.gd, ui/screens/boot_screen.*; Godot 4.7.2 headless import/startup passed in M1 run #5 |
| Android host/APK | CI BUILD VERIFIED; DEVICE TEST PENDING | M1 run #5; artifact 15220028245; 28,431,521-byte ARM64 APK |
| Dream Runtime GDExtension | NOT PRESENT | No source in active tree |
| Runtime GDScript loading on Android | NOT VERIFIED | Must prove in M3 |
| DMod loader/runtime | NOT PRESENT | No source in active tree |
| Registries/events/hooks | PLANNED | PMCS/MODDING_API.md |
| Multiplayer mod capability | PLANNED | PMCS/MODDING_API.md |
| GitHub Actions build pipeline | M1 VERIFIED | .github/workflows/android-m1.yml; run #5 success; APK checks passed |

## Immediate next action
1. User installs and validates the M1 ARM64 debug APK on a physical Android device: launch, visible screen, touch counter/button, background/pause and resume, and UI readability.
2. Record the device result in CURRENT_STATE.md. M1 remains open until user confirmation; CI export alone is not device validation.
3. After M1 device validation, begin M2: a separate minimal C++ GDExtension proof without modifying Godot source.
4. Then complete M3: prove runtime GDScript loading from app-private storage in the exported Android build.
5. Only after M2/M3 should the .dmod package manager and full mod API proceed.

Do not restore all historical experiments automatically. Reuse only reviewed code that passes clean-build and licensing checks.

## 2026-11-27 design update
D-010 resolves the prior language conflict: GDScript is the standard language for ordinary .dmod mods. Packages use mod.json; Lua is not a planned runtime dependency. The game intentionally permits trusted scripts to access Godot APIs and game autoloads without a sandbox. The documented Mod API remains the stable compatibility contract. No implementation status changes as a result of this documentation decision.


## 2026-11-27 design clarification — colony governance and space settlements
Accepted design decisions D-011 and D-012:
- The mayor starts as an adviser and progressively assumes more routine colony management as player oversight decreases. This is gradual and explainable, not simply a mayor selected at game start or an always-on full-autonomy toggle.
- The mayor cannot bypass resource or infrastructure limits. Colonies can still decline or be destroyed, including by future enemy threats.
- Space settlements include functional satellites/probes, large modular orbital stations with docking and player-added modules, and mobile colony ships/megaships capable of serving as planetless colonies when properly equipped.
- Asteroid bases, orbital habitats, industrial platforms, and shipyards are part of the extensible design space.

Documentation/design only. No source code, runtime, simulation, or gameplay implementation was added; the active branch remains documentation-only.

## 2026-11-27 design clarification — mobile-colony fleet autonomy and planetary expansion
Accepted decision D-013:
- A mobile colony has its own mayor and may plan/build varied ship types when its technology, designs, shipyard, workforce, materials, energy, fuel, and logistics permit.
- After roughly 5–6 explored star systems, the mayor may ask for explicit player permission to autonomously colonize additional worlds.
- Automatic planetary colonization is limited to genuinely habitable candidates. The Sun, Mercury, Venus's surface, and gas-giant interiors are excluded as ordinary colony targets. Specialized atmospheric floating habitats, orbital habitats, and suitable moons are separate engineered settlement types.
- Expeditions must pass habitability, capability, supply, and logistics checks; founded colonies receive their own governance and remain vulnerable to real failures.

Documentation/design only. No source code or gameplay implementation was added.


## 2026-11-27 design review update — D-014
The user directed that unspecified gameplay systems be completed using The Final Earth 2 and Spaceflight Simulator as design references. Added detailed planned specifications: GAMEPLAY.md, ROCKETS_AND_PHYSICS.md, UNIVERSE.md, AUTOMATION.md, SPACE_INFRASTRUCTURE.md, and PERFORMANCE_AND_SIMULATION.md. Existing SPACE_EXPANSION.md remains the dedicated expansion specification. D-014 records reference-guided adaptation without copying protected expression. Baseline time policy: game clock pauses when the app is closed; time warp is player-controlled in-game. All remain design-only; no source code or gameplay implementation was added.


## 2026-11-27 PMCS review — mobile usability
A review found that simulation systems were specified in depth, but the mobile interaction model was not. Added MOBILE_UX.md covering progressive disclosure, onboarding, touch-first controls, optional assistance/automation, accessibility, and information hierarchy. Its first-launch flow was initially left open and is now resolved by D-015: guided tutorial followed by free sandbox. Other UX details remain proposals unless separately accepted. No gameplay code was changed.


## 2026-11-27 review consistency fixes
Updated CODE_MEMORY.md to record successful workflow run #18 as the latest CodeGraphContext smoke-test evidence; run #17 is historical. CODE_MAP.md remains explicitly stale relative to later documentation commits and is not regenerated because no engine source exists. Clarified PLAN.md delivery wording: the goal is a complete game, built in testable stages, not a single unreviewed implementation run. No gameplay source code was added.


## 2026-11-27 accepted mobile onboarding decision — D-015
The default first-launch path is a guided tutorial, then a free sandbox. The tutorial is practical, progressive, skippable, and replayable later. Skipping it gives immediate access to the same sandbox and does not lock advanced systems or content. After the tutorial, there is no mandatory campaign. Exact lesson sequence remains to be designed. Documentation only; no source code or gameplay implementation was added.


## 2026-11-27 accepted performance/rendering design — D-016
Accepted D-016 establishes adaptive hybrid building rendering and pixel-scale citizen visual LOD. Buildings remain logical entities while static visuals can be baked into dynamically sized sector caches and combined at distance; dynamic effects and selection data remain separate. Building animation/detail adapts gradually, with automatic and player-controlled quality preferences; selecting a distant building temporarily promotes only its visual detail. Citizens remain persistent individual simulation records; their pixel sprites and visible movement are simplified with distance and disappear only below the visual readability threshold. No gameplay state is deleted or aggregated merely because sprites are not drawn. Numeric thresholds and budgets remain unchosen pending Android ARM64 profiling. Documentation only; no source code or runtime behavior was implemented.


## 2026-11-27 accepted terrain and underground-colony decision — D-017

D-017 accepts a hybrid excavatable-terrain design. Planet surfaces use a simple, readable side-view style with minimal vegetation. Players can excavate persistent, traversable tunnels and chambers and establish underground colonies; nearby/relevant spaces receive the fidelity needed for gameplay, while distant or irrelevant details may be simplified. Terrain edits invalidate only affected regions rather than rebuilding an entire planet. Authoritative terrain, collision, excavation topology, resources, and structures remain separate from visual caches.

Asteroid mining remains a distinct system. Structural stability, cave-ins, support materials, detailed geology, and exact excavation mechanics remain open design questions. Numeric terrain resolution and performance budgets await Android ARM64 profiling. Documentation only; no terrain, mining, or underground-colony implementation was added.


## 2026-11-27 accepted underground-excavation rules — D-018
D-018 resolves the initial rules left open by D-017:
- Ordinary excavated tunnels and chambers are stable by default; no routine cave-in or structural-strength simulation.
- No required support structures, beams, pillars, reinforcement, or automatically generated support decorations.
- All terrain uses the same basic excavation rule; no complex material-specific hardness/digging-resistance simulation.
- Material and deposit differences remain relevant to the resources that can be found and extracted.

These choices intentionally reduce CPU work and implementation complexity. D-017's local terrain updates, persistent traversable underground spaces, authoritative collision/topology/resources, and simplified distant underground detail remain in force. Documentation only; no source code or gameplay implementation was added.


## 2026-11-27 consistency review — D-019 to D-021

Accepted documentation decisions:
- D-019: planet surface construction remains polar/slot-based; underground terrain is a persistent local cell grid addressed by angular sector and radial depth. Connected cells define traversable tunnels/chambers. The old fixed 12-layer proposal is removed; numeric resolution and limits await profiling.
- D-020: first playable v1 contains one starting planet, its Moon, nearby asteroids, and a basic functional orbital station. Interstellar travel, multi-system autonomous fleets/colonization, and full megaship colonies are deferred to v2+ and remain in the long-term vision.
- D-021: flight assistance is enabled by default but manual flight is immediately available; Romanian/English is auto-selected from device locale with English fallback and a Settings override.

Consistency fixes:
- Root README now correctly states GDScript .dmod mods and the separate C++ Dream Runtime GDExtension; Lua is historical only.
- PLAN.md now aligns its Godot/GDExtension boundary and terrain coordinates with D-001, D-002, D-010, and D-019, and distinguishes v1 from later content.
- TEXTURES.md now includes 10 modular station/megaship visual assets; the stated total is 341.
- MOBILE_UX.md records D-021 instead of leaving flight assistance and language selection open.

At the time of this consistency review the repository was documentation-only. That snapshot was superseded by the M1 source and CI build recorded below. No gameplay systems, Dream Runtime, terrain, station, or colony simulation were implemented by that documentation review.

M1 clarification: the default path is the standard official Godot Android export pipeline using the official release and export templates. No custom Godot fork or routine source compilation is required. Any pristine upstream source build must have a concrete documented reason.


## 2026-11-27 — development start authorized
The user approved moving from planning into implementation. The next task is M1 in PMCS/ROADMAP.md: establish the official, unmodified Godot Android export baseline and produce the first minimal APK through GitHub Actions. This was an authorization/next-step record at the time it was written. M1 source and CI export evidence were added later and are recorded below; physical-device testing remains pending.


## 2026-11-27 accepted modular architecture — D-022
D-022 requires a modular-by-default implementation: cohesive domain modules, explicit contracts, single ownership of authoritative state, no circular dependencies, no giant all-purpose managers/catch-all scripts, UI/rendering separated from gameplay authority, focused tests, and module-specific PMCS memory as systems become substantial. The detailed catalog and rules are in PMCS/MODULES.md and PMCS/ARCHITECTURE.md.

This is an accepted design decision, not an implementation claim. The initial M1 foundation now exists; future modules must still be introduced with real vertical slices rather than empty scaffolding.


## 2026-11-27 — M1 first Android build verified in CI
- Added a minimal official Godot 4.7.2-stable project and standard Android export preset. Exact upstream release commit: ed1daf0bf001b61586d9930840f2f1394092c079.
- Added app/main.tscn as a thin composition root, core/app_lifecycle.gd for pause/resume notifications, a focused UI boot screen with Godot version/touch counter/status, and an original SVG project icon.
- Fixed Android export texture configuration by enabling ETC2/ASTC import. Connected the test button pressed signal to the screen handler.
- M1 workflow run #5 succeeded: https://github.com/DreamingbullyYT/my-engine-/actions/runs/46042193393.
- APK: Android arm64-v8a, package com.dreamingbully.dreamengine, 28,431,521 bytes. CI verified package metadata and presence of lib/arm64-v8a/libgodot_android.so. The project icon is configured and the non-fatal missing-icon warning from the earlier build is gone.
- Artifact ID 15220028245; download: https://github.com/DreamingbullyYT/my-engine-/actions/runs/46042193393/artifacts/15220028245. Artifact retention ends 2026-12-27.
- Signing limitation: the workflow creates a temporary debug keystore per run. Until a stable debug-signing secret is configured, a later APK may not install as an update over a prior debug build; uninstall the previous package before installing a newer build.
- **Status:** M1 CI/build portion VERIFIED. Physical-device launch, visible UI, touch, and pause/resume remain PENDING user validation. No gameplay systems or Dream Runtime were implemented in this M1 slice.


## 2026-11-27 — hybrid code memory verified
- CodeGraphContext 0.5.1 remains limited to its supported languages and is not treated as a GDScript parser.
- Added .github/scripts/gdscript_code_map.py using tree-sitter-gdscript 6.1.0; Actions smoke test verified direct local call resolution.
- CodeGraphContext workflow run #145 passed: https://github.com/DreamingbullyYT/my-engine-/actions/runs/46042222729.
- Actual GDScript map for source commit 2f2171cb8198771aadfc188ebfacf83122e5826c contains 2 files, 6 functions, and 0 syntax-error files. The map resolves _ready() and _on_test_touch_pressed() calls to _update_touch_count().
- See PMCS/CODE_MEMORY.md, PMCS/CODE_MAP.md, and PMCS/GDSCRIPT_CODE_MAP.md. Native CodeGraphContext indexing is skipped until native/source files in its supported language set exist.
