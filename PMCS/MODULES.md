# Module Catalog and Modularity Rules

Status: **ACCEPTED ARCHITECTURE — partially implemented.** The catalog remains the design target; only the specific source modules listed in CURRENT_STATE.md exist today.
Decision: D-022.
Scope: Dream Engine and the game built on it.

This document defines the intended module boundaries. It is a design map, not a requirement to create every directory immediately. Add modules when their first vertical slice is implemented; do not generate empty folders or abstractions without a real use.

## 1. Core module catalog

| Module | Owns | Must not own |
|---|---|---|
| `app_bootstrap` | Startup sequence, dependency wiring, scene/root composition, shutdown coordination | Gameplay rules or all-purpose game state |
| `core` | Shared IDs/types, clocks and time policy, configuration, logging/diagnostics, small foundational interfaces | Colony, vehicle, or world-specific behavior |
| `world` | Universe model, star systems, celestial bodies, planet metadata, coordinate transforms, world generation and discovery data | Rendering pixels as authoritative world state |
| `terrain` | Planet surface data, polar surface slots, underground cell chunks/topology, terrain collision and local excavation updates | Colony economy, UI, or asteroid flight physics |
| `mining` | Deposit definitions, extraction operations, yields, mining tools/operations and asteroid-mining rules | General inventory ownership or terrain rendering |
| `simulation` | Fixed/deterministic simulation steps, scheduling, pause/time warp, active and distant simulation tiers | Visual-only LOD decisions or UI state |
| `colony` | Colony state, citizens, needs/jobs, buildings and construction, inventories/resources, production chains, economy, research, policies and mayor governance | Screen layout, sprite rendering, orbital calculations |
| `vehicles` | Vehicle part definitions, craft blueprints, assembly, mass properties, staging, propulsion components, fuel and capability composition | Global orbit solver or UI widget implementation |
| `physics` | Gravity, collisions, orbital propagation, atmospheric flight, trajectory integration, landing and docking physics/contracts | Rocket editor UI or mission progression rules |
| `exploration` | Missions, scans/surveys, discovery progression, exploration rewards/contracts, expedition state | Celestial-body authoritative data or propulsion calculations |
| `infrastructure` | Functional satellites/probes, orbital-station modules, docking facilities, shipyards, mobile habitats and life-support installations | Generic vehicle physics or colony citizen needs implementation |
| `automation` | NPC pilot decisions, cargo route execution, fleet orders, remote-colony automation and permission-gated expansion planning | Bypassing the real resource, physics, habitability or logistics rules |
| `rendering` | Presentation of planets, terrain, buildings, citizens, spacecraft and orbital infrastructure; caches, visual LOD and effects | Authoritative simulation state, economy, collision truth or save ownership |
| `ui` | Screens, panels, controls, dialogs, construction tools, inspectors, HUD and touch interaction | Domain rules or direct mutation that bypasses domain APIs |
| `localization` | Locale selection, translation catalogs, formatting and fallback rules | Gameplay decisions |
| `audio` | Audio routing, music, sound effects and spatial-audio presentation | Simulation or gameplay ownership |
| `save_system` | Save slots, serialization orchestration, schema versions, migrations, recovery and module save coordination | Private duplicate copies of gameplay state |
| `modding` | .dmod packages, manifest/archive validation, dependency resolution, lifecycle, mod API, events, registries, hooks and diagnostics | Owning the base game's domain logic |
| `networking` | Session transport, authority, synchronization and mod-defined message transport when multiplayer is implemented | Gameplay-specific rules or assumptions that all v1 play is networked |
| `platform_android` | Android lifecycle, file-picker integration, permissions, app-private storage and Android-specific glue | General game rules or a fork of Godot |
| `dream_runtime` | Separate C++ GDExtension entry points, native bindings and measured performance-critical native facilities | Reimplementing ordinary GDScript gameplay or modifying Godot source |

The v1 implementation must prioritize the modules needed for the one-planet/Moon/asteroid/station scope. Later systems such as interstellar expansion and multiplayer remain separate modules/features and are not reasons to build unused infrastructure early.

## 2. Suggested source layout

Use this as the initial organization guide, adapting names to Godot conventions where necessary:

```text
project/
  project.godot
  app/
  core/
  world/
    universe/
    celestial_bodies/
    planets/
  terrain/
    surface/
    underground/
  mining/
  simulation/
  colony/
    citizens/
    buildings/
    construction/
    resources/
    production/
    research/
    governance/
  vehicles/
  physics/
  exploration/
  infrastructure/
    satellites/
    stations/
    habitats/
  automation/
    pilots/
    cargo/
    fleets/
  rendering/
  ui/
  localization/
  audio/
  saves/
  mods/
  platform/
    android/
  native/
    dream_runtime/
  tests/
```

This tree is illustrative. Do not create empty directories in M1 simply to match it. Let each directory appear with its first real implementation. Keep assets and data definitions with their owning domain when practical; shared assets belong in a shared location only when genuinely shared.

## 3. Internal structure of a module

Use only the parts a module actually needs. A module may contain:
- domain/model: authoritative data and rules;
- application/service: use-cases and coordination;
- data/resources: definitions and configuration;
- presentation: Godot scenes and visual adapters, where relevant;
- integration: explicit interfaces to other modules;
- tests: unit and focused integration tests.

A small module may remain a handful of files. Do not create one file per trivial function, and do not add layers that only forward calls without adding a boundary, test seam, or meaningful responsibility.

## 4. Dependency and ownership rules

1. **One owner per piece of state.** A citizen, building, vehicle, planet, or save record has one authoritative domain owner. Other modules consume its interface or a deliberate read model; they do not maintain competing truth.
2. **Domain logic is independent of UI.** UI calls domain/application interfaces. Domain modules do not import screens, widgets, or visual nodes.
3. **Rendering is presentation-only.** Renderers read authoritative state and produce visuals/caches. Visual LOD, hidden sprites, or cache eviction must never change gameplay, identity, economy, physics, collision, or saves.
4. **Physics is shared.** Player craft, NPC craft, modded craft, and automated fleets use the same physical constraints and capability checks.
5. **Explicit contracts between modules.** Prefer small typed interfaces and documented data contracts. Use direct calls for request/response operations; use signals/events for notifications and decoupled reactions. Do not use a global event bus as a substitute for clear ownership.
6. **No circular dependencies.** If A needs B and B needs A, extract a narrow shared contract or move orchestration to an application-level coordinator.
7. **No god objects.** Avoid a giant `GameManager`, `WorldManager`, `SimulationManager`, or equivalent script that owns unrelated systems. The bootstrap/composition root may wire modules together, but it must not absorb their business logic.
8. **No catch-all files.** Do not put unrelated gameplay, UI, save, physics, and modding logic into a single script. Split a growing file when it acquires multiple responsibilities or independent change/test reasons; there is no arbitrary line-count target.
9. **No duplicated rules.** A concept such as fuel consumption, resource transfer, citizen identity, or orbital propagation has one authoritative implementation. Other modules call it.
10. **Stable IDs and serialization boundaries.** Persist stable identifiers and versioned data contracts, not scene-tree paths or transient node references as the sole identity.
11. **Mod extension points are explicit.** Registries, events and hooks extend a declared contract. Do not make all internals public merely to avoid designing an interface; trusted mods may access internals by the accepted policy, but the documented API remains the compatibility contract.
12. **Native code is selective.** Keep normal gameplay in GDScript. Move code to the C++ Dream Runtime only for a demonstrated native/platform/performance need with tests and profiling evidence.

## 5. Rules for adding or changing features

For every feature:
1. Identify the owning domain module and its authoritative state.
2. Identify the smallest public/internal contract needed by callers.
3. Keep the implementation and tests next to or clearly associated with that module.
4. Update other modules through their contracts rather than editing their internals as a shortcut.
5. Add or update unit tests for local rules and integration tests for changed boundaries.
6. Update the module note in `PMCS/modules/` once the module has meaningful architecture, invariants, dependencies, or pitfalls.
7. Regenerate the CodeGraphContext map/bundle in Actions when source changes.
8. Record cross-module impact and any API/version migration in PMCS.

When a feature genuinely spans modules, create a small application-level coordinator or a focused integration adapter. Do not merge the modules into one large script.

## 6. Required development workflow

- Implement one small vertical slice at a time; it must compile and have meaningful tests before expanding its scope.
- Prefer adding a new module or extending the correct module over patching a central file.
- A change should touch only the owning module and the minimum necessary contracts/callers/tests.
- Large or risky refactors must be isolated from unrelated feature work.
- Review file growth and responsibility drift during code review. Split files when that improves cohesion and testability, not to satisfy an arbitrary line limit.
- CI must run relevant module tests and integration tests. A module is not complete merely because its script parses.
- Keep PMCS, source, tests, and generated code map consistent with the exact commit.

## 7. Module memory

When a module becomes substantial, create `PMCS/modules/<module>.md` with:
- purpose and ownership;
- authoritative state and key data types;
- entry points/public interfaces;
- dependencies and callers;
- invariants and failure behavior;
- tests and how to run them;
- known limitations and next work;
- last verified source commit.

Do not create notes for modules that do not yet exist. Do not copy entire source files into PMCS; store navigation and architectural knowledge, not duplicate code.

## 8. Acceptance criteria for modularity

A feature/module is acceptable when:
- its responsibility and state ownership are clear;
- it does not depend on UI for core rules;
- it has no accidental circular dependency;
- callers use declared contracts;
- its important behavior has focused tests;
- another module can be changed without rewriting the feature's internals;
- code-map and PMCS references can locate its entry points.

These are intended architecture rules. They do not imply that any listed module or source file already exists.
