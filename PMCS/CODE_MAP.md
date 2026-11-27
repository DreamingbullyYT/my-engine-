# Code Map

Status: no source code is present in the current branch baseline. This file is a placeholder for a generated, evidence-backed map.

## Current tree
- README.md — project entry point.
- PMCS/ — architecture, decisions, requirements, build policy, and agent instructions.

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
