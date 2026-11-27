# Current State

Snapshot: 2026-11-27
Default branch: main
Repository: DreamingbullyYT/my-engine-

## Verified
- Repository exists and is public.
- main and godot-4.7.2-migration were moved to empty-tree commits on 2026-11-27.
- The active tree is documentation-only after the intentional source reset.
- Earlier source code is not present in the current branch tree. Earlier Git commits may still contain recoverable history.
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
| Godot integration | NOT PRESENT | Empty-tree reset |
| Android host/APK | NOT PRESENT | Must rebuild |
| Dream Runtime GDExtension | NOT PRESENT | No source in active tree |
| Runtime GDScript loading on Android | NOT VERIFIED | Must prove in M3 |
| DMod loader/runtime | NOT PRESENT | No source in active tree |
| Registries/events/hooks | PLANNED | PMCS/MODDING_API.md |
| Multiplayer mod capability | PLANNED | PMCS/MODDING_API.md |
| GitHub Actions build pipeline | NOT PRESENT | Must recreate |

## Immediate next action
1. Confirm exact Godot 4.7.x release/tag and upstream commit.
2. Build official unmodified Godot for Android ARM64 through GitHub Actions.
3. Produce a minimal installable host APK.
4. Validate launch, visible rendering, touch, pause/resume on the user's phone.
5. Add a separate minimal GDExtension and prove it loads on device.
6. Prove runtime GDScript loading from app-private storage in an exported Android build.
7. Only then implement the .dmod package manager and full mod API.

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
A second review found that simulation systems are specified in depth, but the mobile interaction model is not. Added MOBILE_UX.md as a PLANNED proposal for progressive disclosure, onboarding, touch-first controls, optional assistance/automation, accessibility, and information hierarchy. It is not yet an accepted new decision. Also identified stale PLAN.md wording about PC sandbox testing, offline time, and fixed mayor modes; these are being aligned with accepted decisions. No gameplay code was changed.


## 2026-11-27 review consistency fixes
Updated CODE_MEMORY.md to record successful workflow run #18 as the latest CodeGraphContext smoke-test evidence; run #17 is historical. CODE_MAP.md remains explicitly stale relative to later documentation commits and is not regenerated because no engine source exists. Clarified PLAN.md delivery wording: the goal is a complete game, built in testable stages, not a single unreviewed implementation run. No gameplay source code was added.
