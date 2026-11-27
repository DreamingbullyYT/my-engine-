# DMod packer

Lua-only DMods do not need a C/C++ compiler, .so, bytecode compiler or APK rebuild. The packer uses only Python's standard library.

## Create a Lua mod

1. Copy templates/dmod-lua/ to a new folder.
2. Edit manifest.json and scripts/main.lua.
3. Run:

    python3 tools/dmod/pack.py path/to/MyMod --output MyMod.dmod

The generated .dmod is a ZIP package. Install it through Dream Engine's mod manager/runtime.

## Runtime modes

- lua: source-only Lua package; no native library.
- native: prebuilt native library at native_library.
- hybrid: Lua entry script plus a native library.

Native/hybrid libraries must be built for the target operating system/ABI. The packer packages an existing library; it does not compile one. The SDK/build pipeline is intended to automate that step for advanced native mod authors.

The Godot 4.7.2 runtime currently validates JSON manifests and uses bounded ZIP member reads. Confirm the engine/runtime version required by the mod before distributing it.
