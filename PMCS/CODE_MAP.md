# CodeGraphContext Code Map

- Repository source status: **NO_ENGINE_SOURCE_CODE_TO_INDEX**
- Last smoke-test commit: `5220564f580714154b5178c36a26a09b415ae926`
- Successful smoke-test run: [GitHub Actions #13](https://github.com/DreamingbullyYT/my-engine-/actions/runs/45988928318)
- Indexer: CodeGraphContext 0.5.1

## Real repository
The active branch contains project documentation and PMCS, but no engine implementation source. Therefore there is no real engine call graph yet. This is intentional and must not be presented as an indexed implementation.

## Verified isolated fixture (not project source)
The CI workflow creates temporary C++ and Lua files under the GitHub runner's temporary directory and indexes them without committing them to this repository.

| Metric | Verified result |
|---|---:|
| Fixture files | 3 |
| Functions | 4 |
| Classes | 0 |
| Graph nodes | 14 |
| Graph edges | 15 |
| C++ call relationship | `calculate()` → `helper()` |
| Bundle export | Passed |

## Current tree
- `README.md` — project entry point.
- `PMCS/` — architecture, decisions, requirements, build policy, agent instructions, and this map.
- `.github/workflows/codegraphcontext.yml` — installs CodeGraphContext, smoke-tests C++/Lua indexing, detects real source, indexes it when present, exports bundles, and publishes this map.
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
