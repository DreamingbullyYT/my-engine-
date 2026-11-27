# CodeGraphContext Code Map

- Status: **PENDING_FIRST_CI_RUN**
- Snapshot: 2026-11-27
- Active baseline: PMCS/specification-only; no engine source is present.

## Current tree
- `README.md` — project entry point.
- `PMCS/` — architecture, decisions, requirements, build policy, and agent instructions.
- `.github/workflows/codegraphcontext.yml` — installs CodeGraphContext 0.5.1, detects source files, indexes when source exists, exports a portable graph bundle, and publishes this readable map.
- `.cgcignore` — excludes build output, generated artifacts, and local secrets from indexing.

## Planned source areas (not created)
| Area | Responsibility | Expected language |
|---|---|---|
| Android host | App entry and lifecycle | Kotlin/Gradle |
| Godot integration | Upstream dependency/build glue | Godot build system |
| Dream Runtime | GDExtension and native API | C++ |
| Scripting | Lua VM and bindings | C/C++/Lua |
| Mod manager | Package validation and lifecycle | C++/Lua |
| Mod API | Events, registries, hooks, capabilities | C++/Lua |
| Tests | Unit/integration fixtures and sample mods | C++/Lua/Python |

## Update rules
When code appears, record actual paths, public classes/functions, key callers/callees, ownership/lifecycle, tests, index version/commit, and unresolved dynamic relationships. Never invent symbols to make this map look complete.

## ChatGPT access note
The committed Markdown map is the portable output this ChatGPT environment can read through the GitHub connection. The `.cgc` database bundle is a downloadable Actions artifact; it does not automatically connect this conversation to the CodeGraphContext MCP server.
