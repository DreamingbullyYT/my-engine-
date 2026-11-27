# Codebase Memory and Indexing Strategy

Date assessed: 2026-11-27  
Status: **Hybrid code-memory pipeline verified on the first real GDScript source.** CodeGraphContext and the dedicated GDScript indexer both pass their CI smoke tests.

## Purpose
Keep a machine-generated structural map of the source tree so coding agents can locate symbols, inspect supported relationships, and understand likely change impact without repeatedly rediscovering the whole repository. PMCS remains the authoritative source for intent, decisions, status, invariants, and known issues. Source and tests remain authoritative for implemented behavior.

## Selected tools

### CodeGraphContext
- Repository: https://github.com/CodeGraphContext/CodeGraphContext
- Installed package: codegraphcontext 0.5.1.
- Used for languages and relationships supported by its installed parser set, especially C/C++ and native Dream Runtime code.
- **Important limitation:** the installed CodeGraphContext language set does not include GDScript. Do not feed .gd files into the supported-source detection and do not claim that its graph understands GDScript.

### Dedicated GDScript structural indexer
- Grammar package: tree-sitter-gdscript 6.1.0, MIT-licensed.
- Script: .github/scripts/gdscript_code_map.py.
- Output: PMCS/GDSCRIPT_CODE_MAP.md.
- Parses .gd files, records script/nested classes and function names/line numbers, and currently resolves direct calls to functions declared in the same script file.
- It deliberately reports only syntax-derived facts. It is not a full semantic analyser and does not yet resolve inheritance, dynamic dispatch, resource references, signal connections, autoloads, or cross-file method calls.

### Other evaluated project
- CodeGraph: https://github.com/codegraph-ai/CodeGraph — a separate project previously evaluated. Do not confuse its license, setup, or capabilities with CodeGraphContext.

## Integration
- .github/workflows/codegraphcontext.yml installs the pinned CodeGraphContext and GDScript grammar packages.
- The workflow retains an isolated C++/Lua fixture smoke test for the CodeGraphContext CLI and graph export. The Lua file is only a parser-tool fixture; it does not change the accepted GDScript-first game/mod scripting decision.
- A separate isolated GDScript fixture checks that the dedicated parser detects the local call relationship calculate() -> helper().
- Repository detection excludes .gd from CodeGraphContext's supported-language scan. If no supported native/script source exists, the workflow records that fact rather than pretending a native graph was generated.
- The dedicated GDScript indexer scans actual .gd project files and emits the separate PMCS/GDSCRIPT_CODE_MAP.md.
- PMCS/CODE_MAP.md combines the GDScript map with the CodeGraphContext native-language report and records the exact source commit and generation time.
- The graph bundle and reports are uploaded as Actions artifacts. The GDScript map and combined map are committed/refreshed on main.
- .cgcignore excludes build output, generated artifacts, local secrets, and other irrelevant material.
- Actions artifacts are retained for 30 days. They are temporary downloadable evidence, not a permanent database service.

## Verified CI and source evidence

### M1 Android ARM64 build
- Workflow run #4: https://github.com/DreamingbullyYT/my-engine-/actions/runs/46041958085.
- Result: **success** on commit a69f04a41b83d3f9929f4141f016a3e1248bee77.
- Official Godot 4.7.2 stable editor and official Android export templates were downloaded and checksum-verified.
- Project import and headless startup passed. The project SVG icon is configured; the previous missing-project-icon export error is gone.
- APK export and validation passed: package ID com.dreamingbully.dreamengine, Android arm64-v8a native library present, APK size 28,431,521 bytes.
- Artifact ID 15220112272; artifact name dream-engine-m1-a69f04a41b83d3f9929f4141f016a3e1248bee77-android-arm64. Expires 2026-12-27.
- Device launch, visible UI, touch, and pause/resume still require user confirmation. A successful CI export is not a physical-device test.

### Hybrid code-memory workflow
- Workflow run #138: https://github.com/DreamingbullyYT/my-engine-/actions/runs/46041958212.
- Result: **success** on commit a69f04a41b83d3f9929f4141f016a3e1248bee77.
- CodeGraphContext 0.5.1 installed; its isolated C++/Lua smoke fixture, call analysis, and .cgc export passed.
- Dedicated GDScript parser smoke fixture passed and verified calculate() -> helper().
- Actual repository map: **2 GDScript files, 6 functions, 0 syntax-error files**.
- Detected same-file call edges: _ready() -> _update_touch_count() and _on_test_touch_pressed() -> _update_touch_count().
- The actual repository currently has no CodeGraphContext-supported C/C++ source, so no real-project .cgc native graph was generated. The C++ graph in this run is from the isolated temporary fixture, not engine source.
- Artifact ID 15219828759 contains the code map and smoke outputs; it expires 2026-12-27.
- The combined PMCS/CODE_MAP.md and PMCS/GDSCRIPT_CODE_MAP.md are committed and tied to the indexed source commit a69f04a41b83d3f9929f4141f016a3e1248bee77.

### Earlier CodeGraphContext validation
- Earlier successful workflow run #18: https://github.com/DreamingbullyYT/my-engine-/actions/runs/45989248995.
- It verified package installation, a temporary C++/Lua fixture, calculate() -> helper(), and portable .cgc export.
- An earlier attempt with cgc index --summarize failed because CodeGraphContext 0.5.1 rejects that option. The current workflow uses cgc index . --force without --summarize; run #138 verifies the corrected command sequence.

## Important ChatGPT boundary
CodeGraphContext exposes an MCP server, but this ordinary ChatGPT conversation does not automatically connect to a local or GitHub Actions-hosted MCP process. The GitHub connection lets this assistant inspect committed repository files and workflow outputs; it does not automatically invoke a separate MCP server. The committed Markdown maps and artifact links are the portable interfaces currently usable here. A future remote MCP setup would require a supported, reachable endpoint and explicit client configuration.

## Retrieval and maintenance rules
1. Treat current source and executable tests as the truth for implementation.
2. Use PMCS/CODE_MAP.md to locate the relevant subsystem, then open the exact current source and tests before editing.
3. For GDScript, consult PMCS/GDSCRIPT_CODE_MAP.md. Treat its call graph as same-file direct-call navigation only.
4. For C/C++ and other supported languages, use the CodeGraphContext map/bundle generated from the exact commit.
5. After a meaningful module change, update its PMCS/modules/<module>.md note when the module is substantial, regenerate the maps, and record the commit/test evidence.
6. Never use a stale map as authority. The map's indexed source commit must match the source being inspected.
7. Do not index credentials, local environment files, build output, or vendored dependencies unless explicitly needed.

## Current limitations and next improvements
- Add cross-file GDScript references, preload/load/resource paths, class inheritance, autoload usage, scene/signal connections, and richer call relationships only when they can be resolved accurately.
- Add native C++ source indexing once Dream Runtime is introduced.
- Inspect actual parser output for every new GDScript syntax feature used by the project; syntax errors must fail the indexing job rather than silently produce a misleading map.
- Consider a durable, commit-addressed archive for .cgc bundles if 30-day Actions artifacts prove insufficient. Do not commit large generated binaries to the source tree by default.
- A live MCP connection is optional; the committed maps are sufficient for the current workflow and must remain usable without MCP.

## Decision
**ADOPT A HYBRID CI-AIDED CODE MEMORY SYSTEM.** Use CodeGraphContext for its supported native/script languages and the dedicated Tree-sitter mapper for GDScript. Keep both out of runtime/build dependencies. PMCS + the committed combined/source-specific maps + exact source/tests + downloadable workflow artifacts form the practical memory system. Reassess coverage as real engine modules are added.
