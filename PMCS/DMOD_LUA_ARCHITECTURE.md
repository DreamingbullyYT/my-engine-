# DMod + Lua architecture

**Status:** Accepted architecture requirement; code in progress, not yet CI-verified  
**Updated:** 2026-11-27  
**Active foundation:** vendored Godot 4.7.2-stable, Dream Engine C++ module

## Goals

- A normal mod author creates and distributes one `.dmod` file.
- Lua mods require no native compiler, no `.so`, no on-device compilation and no APK rebuild.
- Advanced native mods remain possible for unrestricted deep C++ access.
- Both script and native mods use the same named hook-point model and the same four hook stages: `Before`, `Around`, `Replace`, `After`.
- Installing, enabling, disabling or removing a mod does not rebuild the engine or game.
- The package format can evolve without forcing ordinary Lua mod authors to learn native build tooling.

## One package format, multiple execution modes

A `.dmod` is a ZIP-based package with a JSON manifest:

```text
ExampleMod.dmod
├── manifest.json
├── scripts/
│   ├── main.lua
│   └── helpers/
├── assets/
└── native/                 # optional; only for native/hybrid mods
    └── android-arm64-v8a/
        └── libexample.so
```

Manifest runtime modes:

- `lua`: Lua scripts only. This is the default and recommended mode for ordinary mods.
- `native`: prebuilt native library for advanced in-process mods.
- `hybrid`: Lua scripts plus optional native library. Both share one mod identity, dependency graph and lifecycle.

Example Lua manifest:

\u0060\u0060\u0060json
{
  "format_version": 1,
  "id": "author.example-mod",
  "name": "Example Mod",
  "version": "1.0.0",
  "runtime": "lua",
  "entry_script": "scripts/main.lua"
}
\u0060\u0060\u0060

A native/hybrid manifest also declares `native_library: "native/android-arm64-v8a/libexample.so"`; use `runtime: "native"` or `runtime: "hybrid"`. The Godot runtime currently reads these fields from `manifest.json`.

## Lua runtime

- Embed Lua 5.4.8 source (upstream tag `v5.4.8`, commit `6e22fedb74cf0c9b6656e9fce8b7331db847c605`) into the Dream Engine Godot C++ module and compile it into the engine. Do not fetch or compile Lua on the phone.
- Load `.lua` source directly from the validated DMod package virtual filesystem. Do not require per-mod bytecode or generated shared libraries.
- The engine owns the Lua VM lifecycle. Each loaded Lua mod receives its own Lua state initially; shared package/dependency APIs are explicit rather than accidental global-state sharing.
- Provide a documented `dream` API for engine objects, events, resources, scenes, physics, input, rendering and mod metadata as those bindings are integrated.
- Use typed Godot/Dream Engine bindings and lifetime-checked object references; do not expose dangling raw pointers as ordinary Lua values.
- Keep script callbacks on documented engine-safe thread/phase points. Lua states are not concurrently entered from arbitrary render/worker threads. Hook points requiring synchronous execution on other threads remain native-only unless a safe per-thread script design is proven.
- Lua is an authoring/runtime layer, not a security boundary. Mods should still be treated as trusted content. Native code remains explicitly unrestricted in-process code.

## Unified hook contract

Lua and native mods register against the same stable, namespaced hook-point IDs. Hook metadata defines the context schema, result behavior, thread/phase constraints and whether script callbacks are supported.

Stages:

1. `Before`: inspect or mutate the call context before the original operation.
2. `Around`: wrap the remaining chain and optionally call `next()` zero, one or multiple times.
3. `Replace`: replace the original operation; the callback may explicitly call `next()` to fall back.
4. `After`: inspect or mutate the result after the operation/chain completes.

Ordering is descending explicit priority, then deterministic registration order. For `After`, callbacks unwind in reverse order. A script-facing shape should be ergonomic, for example:

```lua
local hooks = dream.hooks

hooks.register("godot.scene.node_added", "Before", function(ctx)
    dream.objects.call(ctx.node, "set_meta", "created_by_mod", true)
end, 10)

hooks.register("godot.physics.server2d.step", "Around", function(ctx, next)
    -- Adjust inputs before the real operation.
    ctx.delta = ctx.delta * 0.5
    next()
end)
```

This is the current intended Lua API shape. The actual registered hook IDs are `godot.main_loop.process`, `godot.main_loop.physics_process`, `godot.physics.server2d.step`, `godot.physics.server3d.step`, `godot.scene.node_added`, and `godot.scene.node_removed`. For Lua hooks, `ctx.node` is an ObjectID integer; use `dream.objects.is_valid/get/set/call` to inspect or invoke methods. The API remains subject to compile/runtime verification.

### Shared dispatch and lifecycle

- One owner-aware hook service stores both native callbacks and Lua callback references.
- Registration is associated with the owning DMod ID and is inactive until successful mod enable.
- Disable removes the mod from future dispatch immediately.
- Unload removes all registrations and releases Lua references before destroying the Lua state or unloading a native library.
- Self-unload is deferred until the active dispatch/callback stack has unwound.
- Hook snapshots must remain valid while dispatch runs; do not hold registry locks while invoking mod code.
- Script exceptions are reported with mod ID, hook ID and traceback. A failing script callback must not unwind through Godot/C++; the hook policy should log and disable that callback or use a documented fail-open fallback.
- Native callback exceptions must never cross the C ABI. Native mods remain responsible for their own threads/resources and must stop them before unload returns.

## Hook coverage

A hook registry alone is not sufficient. The Dream Engine C++ module must insert actual dispatch points into real engine operations. Initial integration targets:

- Main loop/frame lifecycle.
- Scene tree and node lifecycle.
- Physics simulation and collision queries.
- Rendering frame submission and resource creation/update.
- Resource loading/import/decode lifecycle.
- Input dispatch and audio mixing where thread constraints permit.

Each hook point must have tests for stage ordering, original-call behavior, context lifetime, mod disable/unload and re-entrant dispatch. Lua tests must also prove error containment and that no Lua callback is entered from an unsupported thread.

## Native libraries and platform packaging

- Native and hybrid DMods may carry prebuilt libraries for each supported OS/ABI under `native/<platform>-<abi>/`.
- The mod SDK/build tooling should compile and package those libraries automatically. Mod authors should not need to manually manage the internal `.so` layout.
- A Lua-only DMod contains no `.so` at all.
- Android installation never compiles a mod. A native Android mod must already contain a compatible ARM64 library; a Lua-only mod is platform-neutral unless it uses platform-specific APIs.
- The APK/engine binary is rebuilt only when the engine/runtime itself changes, not when a user installs a DMod.

## Implementation order

1. [x] Pin and vendor the Lua 5.4.8 source; embed it in the Godot module build.
2. [x] Add an engine-owned Lua state per loaded script mod and package-source entry execution.
3. [x] Add the shared owner-aware Lua/native hook registry with the four stages and deterministic ordering.
4. [x] Add first real call sites for main-loop process/physics, PhysicsServer2D/3D steps and SceneTree node lifecycle.
5. [ ] Complete package dependency validation, helper-module `require`, script/resource VFS and more engine bindings.
6. [ ] Add renderer/resource/input/audio hook points and a sample `.dmod` smoke test.
7. [ ] Compile and run Lua-only, native and hybrid DMods; verify hook order, errors, unload and hot reload in Actions/Android.
8. [ ] Only then mark production DMod and cross-language hook parity complete.

## Non-goals

- Compiling Lua mods into `.so` files.
- Requiring mod authors to compile code for ordinary gameplay/content mods.
- Running Lua callbacks on arbitrary engine threads without an explicit thread-safe execution design.
- Claiming the Lua API or GDExtension alone provides arbitrary private C++ access.
