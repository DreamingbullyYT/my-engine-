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
