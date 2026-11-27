# Architecture

Status: intended design only; not implemented in the current baseline.

## Layers
1. Android app / game host.
2. Official, unmodified Godot 4.7.x.
3. Dream Runtime GDExtension (C++) for native integration and performance-critical extensions.
4. Dream Mod API: GDScript lifecycle, events, registries, hooks, and game integration.
5. Mod Manager: discovery, validation, dependencies, load order, enable/disable, diagnostics, reload.
6. .dmod packages: GDScript, JSON, scenes/resources, textures, audio, and optional advanced native extensions.

## Boundary rules
### Godot
- Treat upstream Godot as an external dependency; pin an exact release and commit.
- Do not edit upstream source files.
- Use Godot's built-in GDScript runtime for ordinary mod scripts.
- Prove runtime script loading in an exported Android build; editor success is insufficient.
- If a required feature is unavailable through public APIs, document the gap and evaluate an extension/upstream contribution before considering a fork.

### Dream Runtime
- C++ GDExtension owns the native integration boundary and performance-critical facilities.
- Avoid Godot private implementation details.
- Expose stable, versioned interfaces.
- Do not require ordinary mod authors to write or compile C++.
- Native extension loading is advanced and trust-sensitive.

### Scripting and mods
- GDScript is the primary and standard language for ordinary .dmod mods.
- Mods can use Godot APIs and directly access game autoloads/internal state by design. This is trusted code, not a sandbox.
- The documented Dream Mod API is the stable compatibility contract; direct internal access is powerful but may break between versions.
- JSON is for manifests, declarative definitions, configuration, and data.
- Lua is not a planned runtime dependency; previous Lua experiments are historical only.
- Define lifecycle, reload semantics, diagnostics, package validation, and practical resource limits.

### Mod Manager
- Validate manifests and archive structure before executing scripts.
- Reject path traversal and oversized/unsafe ZIP entries.
- Resolve dependencies and deterministic load order.
- Detect missing dependencies, version mismatches, conflicts, duplicate IDs, and cycles.
- Attribute errors to owning mod and source file.
- Require explicit user activation and display a trusted-code warning.

### Host integration
The game deliberately exposes useful integration points/autoloads for world access, entity spawning, input, saves, UI, audio, physics, and session networking. New gameplay systems are possible only when the host provides the required primitives.

## Game-host simulation requirements (planned)
For the colony/space game host, keep these systems as explicit, testable services rather than hard-coding them into UI:
- Colony governance service: tracks player oversight/absence, mayor advice, graduated delegation, policy constraints, decisions, and an auditable action history.
- Colony simulation: supports detailed simulation for active locations and aggregated progression for remote colonies while preserving resource and population constraints.
- Space settlement service: distinguishes satellites, orbital stations, and mobile colony ships. Stations and colony ships are modular, saveable entities with docking, power, life-support, storage, population, and expansion interfaces as applicable.
- Shared entity/vehicle interfaces: NPC pilots and modded craft use declared capabilities and the same physical constraints as player-controlled craft.
- Threat and failure systems must be able to cause genuine loss. Governance may mitigate risk but must not silently make colonies invulnerable.

These are proposed game-host boundaries, not implemented engine modules.

## Hook model
Before, Around/Wrap, Replace/Override, and After. Each hook needs a stable target ID, documented argument/result contract, priority/order, owner ID, error handling, and recursion protection. Replacement is opt-in per target.

## Proposed module boundaries
- dream_runtime — GDExtension entry point, versioning, Godot/native bindings.
- dream_mods — package parsing, validation, dependency resolution, load order, lifecycle.
- dream_api — stable public contracts and game-facing facades.
- dream_events — event bus.
- dream_registry — namespaced extensible registries.
- dream_hooks — hook dispatch and target declarations.
- dream_network — mod networking/serialization facade when host networking exists.
- dream_android — Android-only host/build integration.

These are proposed boundaries, not existing directories. Keep the first implementation small. Do not create a separate Lua VM/module.

## Runtime flow
Host starts Godot → Dream Runtime initializes → Mod Manager discovers and validates .dmod packages → dependency solver creates deterministic load order → GDScript entrypoints are loaded → each mod receives API/context → scripts register listeners/content/hooks → host invokes documented integration points → runtime routes calls and reports mod-attributed errors → save/network layers persist or sync explicitly registered data.

## Autonomous mobile-colony expansion service (planned)
The game host should provide a testable expansion planner used by mobile-colony mayors:
- Fleet planning and ship construction requests, constrained by unlocked designs, technology, shipyard modules, production chains, workforce, materials, components, energy, fuel, and construction time.
- Star-system exploration progression and a configurable first expansion-permission prompt after approximately 5–6 systems have been explored.
- Explicit player permission state for autonomous planetary colonization; no silent expansion before approval.
- A habitability evaluator that distinguishes ordinary habitable planets from extreme worlds and from engineered alternatives such as atmospheric floating habitats, orbital habitats, or settlements on suitable moons.
- Expedition feasibility checks for transport, population, life support, food, water, power, landing/habitat infrastructure, and sustained logistics.
- New-colony founding, assignment of local governance, auditable reports, and player revocation/intervention.
These services must share the same economy, physics, and failure rules as player-directed activity. The approximate system-count threshold is a progression/balance setting, not a hard-coded universal constant.


## Detailed game-system contracts
The game-specific design is specified in dedicated PMCS documents rather than being inferred from engine architecture:
- GAMEPLAY.md: individual citizens, needs, jobs, production chains, research, policies, and colony UI.
- ROCKETS_AND_PHYSICS.md: modular craft, mass/propellant, staging, orbital/atmospheric dynamics, docking, and landing.
- UNIVERSE.md: celestial data, procedural systems, discovery, surveys, and interstellar travel.
- AUTOMATION.md: mayor delegation, NPC piloting, fleet orders, cargo routes, and remote simulation.
- SPACE_INFRASTRUCTURE.md: functional satellites, connected station modules, megaships, and habitats.
- SPACE_EXPANSION.md: permission-gated expansion and habitability checks.
- PERFORMANCE_AND_SIMULATION.md: simulation fidelity tiers, save integrity, time policy, Android lifecycle, and profiling.
These are design contracts only. Implementation must be proven with tests and device evidence.
