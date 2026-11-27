# Dream Engine Godot module

The built-in module is compiled into Godot's source-built editor/runtime and registers the `DreamMods` singleton.

Current implementation scope:
- Lua 5.4.8 is compiled into the engine from `third_party/lua-5.4.8`.
- Lua-only `.dmod` ZIP packages use `manifest.json` and a source `entry_script`.
- Each loaded Lua mod owns a separate Lua state.
- Both Lua and C++ native callbacks use the same named hook registry and Before/Around/Replace/After ordering.
- Godot main-loop process, physics callbacks, PhysicsServer2D/3D steps, and SceneTree node-added/node-removed events are connected through the patch set under `patches/godot/`.
- Hook contexts are mutable dictionaries. Lua receives primitive values and object instance IDs; native callbacks receive the Godot Dictionary and may query raw engine singletons through the versioned host API.

The DMod runtime is trusted in-process code. Lua standard libraries are opened; native code is not sandboxed. Do not load untrusted mods.

The native ABI hook registration surface is present, but package extraction/loading of native shared libraries and full hybrid mode are still in progress. Do not claim those modes are complete until the native loader, tests, and Android CI are verified.
