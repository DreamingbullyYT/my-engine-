# Dream Engine Code Map

- Source commit: 18312379cc8045986a512e284116f199fb69eb15
- Generated: 2026-11-28T11:07:57Z

## GDScript
# GDScript Code Map

- Source commit: 18312379cc8045986a512e284116f199fb69eb15
- Generated (UTC): 2026-11-28T11:07:57Z
- Parser: tree-sitter-gdscript 6.1.0
- Files: **11**
- Script/nested classes: **0**
- Functions: **86**
- Files with syntax errors: **0**

> Syntax-derived navigation only. Source and tests remain authoritative.
> Call edges are included only for direct calls to functions declared in the same file.

## colony/colony_simulation.gd

- Functions:
  - _ready() — line 61; calls start_new_game()
  - start_new_game() — line 71; calls _assign_workers(), _set_message()
  - can_spend() — line 97
  - try_spend() — line 104; calls can_spend()
  - can_build() — line 113; calls can_spend()
  - build() — line 120; calls _assign_workers(), _set_message(), try_spend()
  - has_building() — line 143
  - get_housing_capacity() — line 150
  - get_employed_count() — line 159
  - get_average_happiness() — line 167
  - advance_day() — line 176; calls _set_message()
  - get_population_summary() — line 231; calls get_average_happiness(), get_employed_count(), get_housing_capacity()
  - get_snapshot() — line 240
  - restore_snapshot() — line 250; calls _assign_workers(), _set_message()
  - _assign_workers() — line 288
  - _set_message() — line 318

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

## rendering/planet_view.gd

- Functions:
  - set_world() — line 11
  - _draw() — line 18; calls _draw_building(), _draw_citizens(), _draw_rocket()
  - _draw_building() — line 49
  - _draw_citizens() — line 75
  - _draw_rocket() — line 83

## saves/save_service.gd

- Functions:
  - save_game() — line 7
  - load_game() — line 33

## tests/gameplay_smoke.gd

- Functions:
  - _initialize() — line 9; calls _check()
  - _check() — line 59

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

## ui/screens/game_screen.gd

- Functions:
  - _ready() — line 17; calls _build_interface(), _refresh()
  - _build_interface() — line 28; calls _make_button()
  - _make_button() — line 123
  - _on_build_pressed() — line 133; calls _refresh()
  - _on_build_rocket_pressed() — line 138; calls _on_message_changed(), _refresh()
  - _on_launch_pressed() — line 150; calls _on_message_changed(), _refresh()
  - _on_return_pressed() — line 156; calls _on_message_changed(), _refresh()
  - _on_save_pressed() — line 162; calls _on_message_changed()
  - _on_load_pressed() — line 167; calls _on_message_changed(), _refresh()
  - _on_diagnostics_pressed() — line 185
  - _on_message_changed() — line 189
  - _on_application_paused() — line 194; calls _on_message_changed()
  - _on_application_resumed() — line 198; calls _on_message_changed()
  - _refresh() — line 202

## vehicles/rocket_flight.gd

- Functions:
  - _process() — line 29; calls advance_simulation()
  - construct() — line 39; calls _emit_message()
  - can_launch() — line 51
  - can_return() — line 55
  - is_unbuilt() — line 59
  - is_on_surface() — line 63
  - is_in_orbit() — line 67
  - launch() — line 71; calls _emit_message(), can_launch()
  - begin_return() — line 82; calls _emit_message(), can_return()
  - advance_simulation() — line 93; calls _step_physics()
  - get_altitude() — line 101
  - get_orbital_speed() — line 105
  - get_circular_speed() — line 109
  - get_orbital_period_seconds() — line 113
  - get_status_text() — line 118; calls get_altitude(), get_orbital_period_seconds(), get_orbital_speed()
  - get_snapshot() — line 135
  - restore_snapshot() — line 147
  - _step_physics() — line 169; calls _emit_message(), get_circular_speed()
  - _emit_message() — line 201

## CodeGraphContext (supported native/script languages)
# CodeGraphContext Code Map

- Commit: 18312379cc8045986a512e284116f199fb69eb15
- Generated: 2026-11-28T11:07:54Z
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
