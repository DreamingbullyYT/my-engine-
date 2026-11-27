# Hello DMod (Lua-only)

This is a source-only mod. Edit manifest.json and scripts/main.lua, then package it with the DMod packer:

    python3 tools/dmod/pack.py templates/dmod-lua --output HelloDMod.dmod

It contains no native library and requires no compiler. The example registers a Before hook against the main-loop process call.
