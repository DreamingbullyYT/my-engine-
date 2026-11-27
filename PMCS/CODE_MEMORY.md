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


## Next evolution: persistent code knowledge for the implementation phase
The existing CodeGraphContext workflow is the starting system; do not replace it without evidence. The intended code-memory loop is:
1. **Authoritative source:** current repository code, tests, build configuration, and PMCS decisions. Generated memory must never override these.
2. **Structural index:** CodeGraphContext graph of files, symbols, and supported call/dependency relationships, generated from the exact commit being indexed.
3. **Portable agent-readable map:** concise PMCS/CODE_MAP.md with repository/module layout, important entry points, symbol relationships, high-complexity areas, and test locations. Include the indexed commit SHA and generation time.
4. **Focused module memory:** as real modules appear, maintain small PMCS/modules/*.md notes for architecture, invariants, public interfaces, lifecycle, dependencies, and known pitfalls. Update these alongside code changes; avoid duplicating whole source files.
5. **Verification trail:** Actions should publish the graph bundle and logs as artifacts, run indexer smoke tests, and ensure the generated map corresponds to the checked-out commit. Current artifact retention is 30 days, so artifacts are temporary evidence, not permanent memory.
6. **Safe retrieval rule:** before editing, use the map/graph to locate relevant symbols, then inspect the exact current source and tests. After editing, update affected module notes and regenerate the map. Never infer implementation details from stale maps or old commits.

### Gaps to address when source is introduced
- The current workflow smoke-tests C++ and Lua using an isolated fixture; the accepted game scripting language is GDScript. Add a GDScript indexing capability test before relying on graph coverage for .gd source, and retain C++ coverage for the GDExtension.
- Confirm which CodeGraphContext outputs are genuinely useful on the real Godot/GDScript/C++ tree; report unsupported relationships rather than implying complete semantic understanding.
- Consider a durable, commit-addressed archive for .cgc bundles if 30-day Actions artifacts prove insufficient. Do not commit large generated binaries to the source tree by default.
- The current conversation's GitHub connection can read committed files and Actions metadata; it is not a direct live connection to CodeGraphContext's MCP server. A future remote MCP setup is optional and requires a supported reachable endpoint and explicit connection.

### Current conclusion
**Keep CodeGraphContext as the experimental CI structural indexer.** Improve it incrementally once real source exists. The practical memory system is the combination of source + tests + PMCS + generated map + graph artifact, not the graph alone.
