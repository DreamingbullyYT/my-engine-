# Lua 5.4.8 source

- Upstream mirror: https://github.com/lua/lua
- Release tag: `v5.4.8`
- Exact commit: `6e22fedb74cf0c9b6656e9fce8b7331db847c605`
- License: MIT, see [LICENSE](LICENSE).

The Lua core and standard-library sources are included through `onelua.c` and built into the Dream Engine Godot module. The build does not define `MAKE_LUA` or `MAKE_LUAC`, so it does not create the standalone interpreter or bytecode compiler. Mod packages contain Lua source; they do not contain compiled bytecode or shared libraries.
