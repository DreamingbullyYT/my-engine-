# Codebase Memory and Indexing Strategy

Date assessed: 2026-11-27  
Status: **CodeGraphContext installed and isolated C++/Lua indexing smoke test passed.** Real project indexing is pending engine source.

## Purpose
Keep a machine-generated structural map of the source tree so coding agents can locate symbols, inspect dependencies, and understand likely change impact without repeatedly rediscovering the whole repository. PMCS remains the authoritative source for intent, decisions, status, and known issues.

## Selected tool
- **CodeGraphContext**: https://github.com/CodeGraphContext/CodeGraphContext — MIT-licensed Python CLI + MCP server. Selected for the first experiment because it supports C++, Lua, graph analysis, and portable graph bundles.
- **CodeGraph**: https://github.com/codegraph-ai/CodeGraph — a separate project previously evaluated. Do not confuse its license, setup, or capabilities with CodeGraphContext.
- CI installs the pinned PyPI package `codegraphcontext==0.5.1`.

## Integration
- `.github/workflows/codegraphcontext.yml` installs CodeGraphContext on GitHub Actions.
- It runs an isolated temporary C++/Lua fixture smoke test on every workflow run. The fixture exists only under the runner temporary directory; it is not committed as engine code.
- It detects whether actual project source files exist. It does not pretend Markdown-only PMCS is an engine implementation.
- When actual source exists, it builds a graph, emits a readable `PMCS/CODE_MAP.md`, and exports a portable `.cgc` bundle.
- The readable map is committed/refreshed on `main`; graph and smoke-test outputs are uploaded as workflow artifacts.
- `.cgcignore` excludes build output, generated artifacts, local secrets, and other irrelevant material.
- Workflow artifacts are retained for 30 days. They are not a permanent database service.

## Verified isolated smoke test
- Workflow run **#18**: https://github.com/DreamingbullyYT/my-engine-/actions/runs/45989248995 (latest verified successful run, 2026-11-27).
- Result: **success**. Run #17 below is historical; #18 supersedes it as the latest verification.
- CodeGraphContext 0.5.1 installed successfully on GitHub-hosted Ubuntu.
- Temporary fixture: 3 files (C++ source/header + Lua source).
- Graph statistics: **3 files, 4 functions, 0 classes**.
- Call analysis verified: `calculate()` calls `helper()` in the C++ fixture.
- Bundle export succeeded: **14 nodes, 15 edges**.
- Artifact **15192079735** contains the `.cgc` smoke graph and CLI logs; it is retained until 2026-12-27.
- The fixture and graph were generated in the runner temp directory, not added to project source.
- Earlier smoke run #12 failed because the CLI rejected `--summarize` on `cgc index`; this was removed. Run #13 validated the corrected command sequence.
- The current `PMCS/CODE_MAP.md` still contains an older source commit identifier and is not a current generated map. Since the repository remains documentation-only, do not fabricate a new generated source map; regenerate it when source files exist.

## Important ChatGPT boundary
CodeGraphContext exposes an MCP server, but this ordinary ChatGPT conversation does not automatically connect to a local or GitHub Actions-hosted MCP process. The GitHub connection lets this assistant inspect committed repository files; it does not automatically invoke a separate MCP server. The committed Markdown map is the portable interface currently usable here. A future remote MCP connection would require a supported, reachable MCP endpoint and explicit client configuration.

## Current repository baseline
The active repository tree is still specification-only after the intentional source reset. The smoke test proves that CodeGraphContext can install, parse a controlled C++/Lua fixture, resolve at least one call relationship, and export a graph. It does **not** prove that the real Dream Engine source has been indexed or implemented.

## Adoption criteria
- Keep the smoke test in CI so tool regressions are detected.
- When engine source is added, verify real project indexing and inspect generated statistics and important caller/callee relationships.
- Keep generated facts tied to the exact commit SHA.
- Never treat the graph as authoritative over current source or tests; inspect exact source ranges before edits.
- Do not index credentials, local environment files, build output, or vendored dependencies unless explicitly needed.

## Decision
**ADOPT AS AN EXPERIMENTAL CI-AIDED INDEXER.** Keep it out of runtime/build dependencies. PMCS + committed `PMCS/CODE_MAP.md` is the portable interface for this ChatGPT environment. Reassess after indexing the first real engine source.
