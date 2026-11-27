# CodeGraphContext Code Map

- Status: **NO_SOURCE_CODE_TO_INDEX**
- Indexed commit: `8f0ac7317c3d1c3e223edb7541c15ca4dad0f053`
- Successful CI run: [45987570118](https://github.com/DreamingbullyYT/my-engine-/actions/runs/45987570118)
- Report artifact generated: 2026-11-27 11:06:21 UTC
- Indexer: CodeGraphContext 0.5.1

## Verified result
- CodeGraphContext installed successfully on GitHub Actions.
- Source detection correctly found no implementation source in the active branch.
- Indexing and graph export were correctly skipped because the active tree contains only project documentation.
- A readable status report artifact was uploaded.
- The initial report had empty commit/time fields due to shell backtick substitution. The workflow was corrected after this run; that metadata correction still needs validation on a later run.

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

## ChatGPT access note
The committed Markdown map is the portable output this ChatGPT environment can read through the GitHub connection. The `.cgc` database bundle is a downloadable Actions artifact; it does not automatically connect this conversation to the CodeGraphContext MCP server.
