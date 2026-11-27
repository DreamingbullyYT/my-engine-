# Codebase Memory and Indexing Strategy

Date assessed: 2026-11-27  
Status: candidate evaluation; no memory server is installed or connected.

## Problem to solve
The agent should not repeatedly scan every source file to reconstruct architecture. It needs a persistent, queryable index of:
- files, symbols, classes, methods, and public APIs;
- imports, callers/callees, inheritance, and module dependencies;
- related tests and likely change impact;
- project decisions, known bugs, debugging findings, and design documents;
- freshness information tied to a Git commit.

This is code intelligence, not just a general chat-memory feature.

## Best candidate found: CodeGraph
Repository: https://github.com/codegraph-ai/CodeGraph

Why it matches:
- Builds a semantic code graph and exposes it through MCP.
- Advertises C++ and Lua language parsing, both relevant to Dream Runtime/mod scripting.
- Has persistent project memory for debugging notes, decisions, and known issues.
- Has documentation indexing/search and design-to-code gap checks.
- Provides graph/context tools for callers, callees, tests, dependencies, and impact.
- Its repository declares Apache-2.0 licensing.
- Includes a GitHub Actions example for graph-only PR analysis.

Important limitation:
- CodeGraph is an external code-intelligence tool/MCP server, not a ChatGPT plugin that was found in the available plugin directory.
- Its normal MCP setup expects a running local server/client. Do not assume ChatGPT can connect to it directly in this environment.
- GitHub Actions runners are temporary. A CI workflow must explicitly persist the generated database/cache or export useful Markdown/JSON artifacts; otherwise the index disappears when the runner ends.
- Verify current C++ parser quality and exact build/install steps before adopting it.

## Other candidate: CodeAtlas
Repository: https://github.com/priy-anshugupta/CodeAtlas

It describes a persistent local repository map, task-specific context packets, dependency/impact queries, CLI/MCP integrations, and a dashboard. It is also a candidate, but CodeGraph currently appears the closer match because its published language list explicitly includes C++ and Lua and it describes a dedicated persistent memory layer.

## ChatGPT plugin search result
The available plugin search did not return a dedicated codebase-memory/indexing plugin. GitHub is already connected and can inspect repository files. Context7 can supply up-to-date external library documentation, but it is not a persistent index of this project's own source code. Notion can store project notes, but it does not replace a symbol/call graph.

## Recommended integration for this project
1. Keep PMCS as the human-readable source of truth for intent, decisions, status, and workflow.
2. When implementation code exists, evaluate CodeGraph on a small representative C++/Lua sample.
3. Add a GitHub Actions workflow that indexes the repository on push/PR.
4. Persist the index between runs using a controlled artifact/cache strategy, keyed by branch and commit; never store secrets in it.
5. Export a compact deterministic report to PMCS/generated-code-map/ (or a workflow artifact): module map, public symbols, dependencies, call graph highlights, test links, and index commit SHA.
6. Make every coding agent read PMCS first, then query the index for the task-specific context. Read exact source ranges before edits.
7. Refresh the index after changes and invalidate stale facts when symbols/files disappear.

## Acceptance criteria before adoption
- Works with the project's C++ and Lua sources.
- Reproducible on GitHub Actions Linux runners.
- Incremental update is correct after file edits/deletes/renames.
- Index is tied to exact commit SHA.
- Does not index secrets, build output, generated binaries, or vendored dependencies unless explicitly needed.
- Produces useful context for a real task and identifies callers/tests without requiring a full-repository prompt dump.
- License and dependency licenses are compatible with project usage.
- The agent can actually query the generated output in its environment; a database artifact alone is not useful if no tool can read it.

## Decision
**PROVISIONAL RECOMMENDATION: test CodeGraph first.** Do not add it as a mandatory dependency or claim it is connected until a working end-to-end proof exists. Keep PMCS + generated Markdown/JSON map as the portable fallback.
