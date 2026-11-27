# Dream Engine Code Map

- Source commit: 820fe97a3b684f3fd379373f011edaada40cca0e
- Generated: 2026-11-27T19:37:07Z

## GDScript
# GDScript Code Map

- Source commit: 820fe97a3b684f3fd379373f011edaada40cca0e
- Generated (UTC): 2026-11-27T19:37:07Z
- Parser: tree-sitter-gdscript 6.1.0
- Files: **4**
- Script/nested classes: **0**
- Functions: **14**
- Files with syntax errors: **0**

> Syntax-derived navigation only. Source and tests remain authoritative.
> Call edges are included only for direct calls to functions declared in the same file.

## core/app_lifecycle.gd

- Functions:
  - _notification() — line 10

## mods/mod_runtime.gd

- Functions:
  - run_external_probe() — line 14; calls _fail()
  - report_external_mod_result() — line 53; calls _fail()
  - _fail() — line 61

## tests/native_extension_smoke.gd

- Functions:
  - _initialize() — line 8
  - _run_native_probe() — line 12

## ui/screens/boot_screen.gd

- Functions:
  - _ready() — line 12; calls _check_native_runtime(), _update_touch_count()
  - _check_native_runtime() — line 23
  - _on_test_touch_pressed() — line 50; calls _update_touch_count()
  - _on_test_external_script_pressed() — line 56
  - _on_external_probe_finished() — line 61
  - _on_application_paused() — line 69
  - _on_application_resumed() — line 73
  - _update_touch_count() — line 77

## CodeGraphContext (supported native/script languages)
# CodeGraphContext Code Map

- Commit: 820fe97a3b684f3fd379373f011edaada40cca0e
- Generated: 2026-11-27T19:37:03Z
- Indexer: CodeGraphContext 0.5.1

## Index statistics
Resolving context...
Initializing services and database connection...
Services initialized.
📊 Statistics for: /home/runner/work/my-engine-/my-engine-

┏━━━━━━━━━━━━━━━━━━┳━━━━━━━┓
┃ Metric           ┃ Count ┃
┡━━━━━━━━━━━━━━━━━━╇━━━━━━━┩
│ Files            │    34 │
│ Functions        │     9 │
│ Classes          │     1 │
│ Imported Modules │     7 │
└──────────────────┴───────┘

## High-complexity functions (threshold 10)
Resolving context...
Initializing services and database connection...
Services initialized.

## Potential dead code
Resolving context...
Initializing services and database connection...
Services initialized.
