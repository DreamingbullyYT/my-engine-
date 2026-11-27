# Lua 5.4.8 source

- Upstream mirror: https://github.com/lua/lua
- Release tag: `v5.4.8`
- Exact commit: `6e22fedb74cf0c9b6656e9fce8b7331db847c605`
- License: MIT, see [LICENSE](LICENSE).

The source headers and `onelua.c` are built into the Dream Engine Godot module. The local `onelua.c` adds `#define MAKE_LIB` so it builds the Lua core and standard libraries without a standalone interpreter entry point. Mod packages contain Lua source; they do not contain compiled bytecode or shared libraries.
