# Codebase Memory and Indexing Strategy

Date assessed: 2026-11-27  
Status: CodeGraphContext CI integration added; first run pending.

## Purpose
Keep a machine-generated structural map of the source tree so coding agents can locate symbols, inspect dependencies, and understand likely change impact without repeatedly rediscovering the whole repository. PMCS remains the authoritative source for intent, decisions, status, and known issues.

## Distinguish the projects
- **CodeGraphContext**: https://github.com/CodeGraphContext/CodeGraphContext — MIT-licensed Python CLI + MCP server. This is the selected first experiment. It supports C++, Lua, and several other languages. Its documented CLI includes indexing, statistics, complexity analysis, dead-code analysis, and portable graph bundle export.
- **CodeGraph**: https://github.com/codegraph-ai/CodeGraph — a separate project previously evaluated as a candidate. Do not confuse its license, setup, or capabilities with CodeGraphContext.

## Integration now present
- `.github/workflows/codegraphcontext.yml` installs the pinned PyPI package `codegraphcontext==0.5.1` on GitHub Actions.
- It detects whether source files exist. It does not pretend Markdown-only PMCS is an engine implementation.
- When source exists, it builds a graph, emits a readable `PMCS/CODE_MAP.md`, and exports a portable `.cgc` bundle.
- The readable map is uploaded as a workflow artifact and refreshed on `main` by the workflow.
- `.cgcignore` excludes build output, generated artifacts, local secrets, and other irrelevant material.
- The graph bundle artifact is retained for 30 days. It is not a permanent database service.

## Important ChatGPT boundary
CodeGraphContext exposes an MCP server, but the MCP setup instructions for ChatGPT Codex do not mean this ordinary ChatGPT conversation can automatically connect to a local or GitHub Actions-hosted MCP process. The GitHub connector lets this assistant inspect committed repository files; it does not automatically invoke a separate MCP server. Therefore the portable, committed Markdown map is the immediately usable bridge. A future remote MCP connection would need a supported, reachable MCP endpoint and explicit client configuration.

## Current baseline
The active repository tree is currently specification-only after the intentional source reset. Consequently, the first workflow run should report `NO_SOURCE_CODE_TO_INDEX`. This is expected, not a workflow failure. Actual graph indexing begins when implementation source is committed.

## Adoption criteria
- Verify the first GitHub Actions run installs CodeGraphContext successfully.
- Once source exists, verify indexing, stats, complexity/dead-code reports, and `.cgc` export.
- Inspect the generated report against real C++/Lua code and manually verify important caller/callee relationships.
- Keep generated facts tied to the exact commit SHA.
- Never treat the graph as authoritative over current source or tests; inspect exact source ranges before edits.
- Do not index credentials, local environment files, build output, or vendored dependencies unless explicitly needed.

## Decision
**ADOPT AS AN EXPERIMENTAL CI-AIDED INDEXER.** Keep it out of runtime/build dependencies. PMCS + committed `PMCS/CODE_MAP.md` is the portable interface for this ChatGPT environment. Reassess after the first successful CI run and after real source code is present.
