# Architecture

Status: intended design only; not implemented in the current baseline.

## Layers
1. Android app / game host.
2. Official, unmodified Godot 4.7.x.
3. Dream Runtime GDExtension (C++).
4. Dream Mod API: lifecycle, events, registries, hooks, capabilities, host facades.
5. Mod Manager: discovery, validation, dependencies, load order, enable/disable, diagnostics, reload.
6. .dmod packages: Lua scripts, JSON data, scenes/resources, textures/models/audio, optional native extension.

## Boundary rules

### Godot
- Treat upstream Godot as an external dependency.
- Pin an exact release and commit.
- Do not edit upstream source files.
- If a required feature is unavailable through public GDExtension APIs, document the gap and evaluate a separate extension or upstream contribution before considering a fork.

### Dream Runtime
- C++ GDExtension owns the native integration boundary.
- Avoid Godot private implementation details.
- Expose stable, versioned interfaces to scripts.
- Do not expose raw engine internals as the default mod API.
- Native extension loading is advanced and trust-sensitive.

### Scripting
- Lua is the planned normal mod scripting language.
- Lua scripts use explicit Dream API bindings, not arbitrary private engine state.
- JSON is for manifests, declarative definitions, configuration, and data.
- Define lifecycle, reload semantics, diagnostics, and practical resource limits.
- Lua is not automatically a security sandbox.

### Mod Manager
- Validate manifests before executing scripts.
- Resolve dependencies and deterministic load order.
- Detect missing dependencies, version mismatches, conflicts, duplicate IDs, and dependency cycles.
- Attribute errors to the owning mod and source file.

### Host integration
A game must deliberately expose capabilities such as world access, entity spawning, input actions, save data, and session networking. A mod cannot assume every game exposes every capability.

## Hook model
Planned stages:
- Before — run before the target.
- Around — wrap the target and optionally call the original.
- Replace — replace only explicitly replaceable targets.
- After — run after the target.

Each hook needs a stable target ID, typed arguments/results, priority/order, owner mod ID, error handling, and recursion protection. Replacement is opt-in per target.

## Proposed module boundaries
- dream_runtime — GDExtension entry point, versioning, Godot bindings.
- dream_scripting — Lua VM lifecycle and bindings.
- dream_mods — package parsing, dependency resolution, load order.
- dream_api — stable public contracts.
- dream_events — event bus.
- dream_registry — namespaced registries.
- dream_hooks — hook dispatch and target declarations.
- dream_network — mod networking and serialization facade.
- dream_android — Android-only host/build integration.

These are proposed boundaries, not existing directories. Keep the first implementation small; split only when ownership or dependency boundaries justify it.

## Runtime flow
Host starts Godot → Dream Runtime initializes → Mod Manager discovers and validates packages → dependency solver creates deterministic load order → each mod receives a context → scripts register listeners/content/hooks → host invokes documented integration points → runtime routes calls and reports mod-attributed errors → save/network layers persist or sync explicitly registered data.
