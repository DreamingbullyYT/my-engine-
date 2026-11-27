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
