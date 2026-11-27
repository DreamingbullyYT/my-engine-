# Dream Engine Code Map

- Source commit: 00233bb934cd31329a5825ab513ce0d163c9dcf3
- Generated: 2026-11-27T20:52:28Z

## GDScript
# GDScript Code Map

- Source commit: 00233bb934cd31329a5825ab513ce0d163c9dcf3
- Generated (UTC): 2026-11-27T20:52:28Z
- Parser: tree-sitter-gdscript 6.1.0
- Files: **5**
- Script/nested classes: **0**
- Functions: **28**
- Files with syntax errors: **0**

> Syntax-derived navigation only. Source and tests remain authoritative.
> Call edges are included only for direct calls to functions declared in the same file.

## core/app_lifecycle.gd

- Functions:
  - _notification() — line 10

## mods/mod_package_manager.gd

- Functions:
  - create_and_run_sample() — line 18; calls _finish(), install_and_enable()
  - install_and_enable() — line 56; calls _finish(), _is_safe_archive_path(), _remove_tree(), _validate_manifest(), enable_mod()
  - enable_mod() — line 185; calls _finish(), enable_mod()
  - disable_mod() — line 265; calls _finish()
  - report_dmod_result() — line 283; calls _finish()
  - _validate_manifest() — line 291; calls _is_safe_archive_path(), _valid_mod_id(), _valid_version()
  - _valid_mod_id() — line 313
  - _valid_version() — line 319
  - _is_safe_archive_path() — line 325
  - _remove_tree() — line 334; calls _remove_tree()
  - _finish() — line 348

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
  - _ready() — line 13; calls _check_native_runtime(), _update_touch_count()
  - _check_native_runtime() — line 25
  - _on_test_touch_pressed() — line 52; calls _update_touch_count()
  - _on_test_external_script_pressed() — line 58
  - _on_external_probe_finished() — line 63
  - _on_install_sample_mod_pressed() — line 71
  - _on_package_finished() — line 76
  - _on_disable_sample_mod_pressed() — line 81
  - _on_application_paused() — line 86
  - _on_application_resumed() — line 90
  - _update_touch_count() — line 94

## CodeGraphContext (supported native/script languages)
# CodeGraphContext Code Map

- Commit: 00233bb934cd31329a5825ab513ce0d163c9dcf3
- Generated: 2026-11-27T20:52:26Z
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
