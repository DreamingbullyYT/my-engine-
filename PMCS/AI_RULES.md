# Rules for AI Coding Agents

Read PMCS before modifying this repository.

## Non-negotiable
1. Inspect current branch, status, files, and recent commits first. The source branches were intentionally emptied; do not assume old files exist.
2. Current source/tests define implemented behavior. PMCS defines intended behavior.
3. Never claim a file, class, API, test, APK, or feature exists unless verified.
4. Never fabricate CI runs, artifacts, SHAs, test results, or device outcomes.
5. Do not restore historical code wholesale. Review its commit, license, dependencies, and tests.
6. Keep official Godot source unmodified. Dream functionality belongs in separate GDExtension/runtime code.
7. Do not introduce a Godot fork unless extension APIs are proven insufficient and the user approves.
8. Design every dependency/build choice for Android ARM64 and GitHub Actions.
9. Prefer small modular patches and separate files.
10. Add tests with every behavior change.
11. Update PMCS in the same change when design, API, status, build, or decisions change.
12. Mark missing information UNKNOWN; do not guess.
13. Ordinary .dmod mods use GDScript. Do not reintroduce Lua as the planned standard runtime without a new explicit user decision.
14. Treat script and native mods as trusted code; never claim they are sandboxed.

## Required workflow
Before editing: read PMCS, inspect branch/status/history, identify call sites/dependencies/tests, and define the smallest safe patch.

While editing: preserve public contracts unless intentionally versioned; avoid duplicated systems and unnecessary dependencies; add failure-path tests.

Before reporting completion: run relevant tests/builds, inspect final diff, verify artifacts exist and are non-zero, update CURRENT_STATE.md with evidence, and separate complete/partial/blocked/untested work.

## Codebase memory workflow
Maintain a generated code map of files, symbols, dependencies, call relationships, tests, and impact hints alongside PMCS. Evaluate indexing tools for language coverage, license, GitHub Actions compatibility, reproducible output, and whether ChatGPT can actually query the generated index. Do not claim an MCP server is connected unless configured and reachable.

Until then, keep PMCS/CODE_MAP.md updated, add module READMEs for non-obvious subsystems, record public API symbols and paths, and include change-impact notes for cross-module changes.
