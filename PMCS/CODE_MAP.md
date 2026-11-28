# Dream Engine Code Map

- Source commit: 82873e2601c5d4f0da934a1ec4ae3d8343c65afc
- Generated: 2026-11-28T11:54:20Z

## GDScript
# GDScript Code Map

- Source commit: 82873e2601c5d4f0da934a1ec4ae3d8343c65afc
- Generated (UTC): 2026-11-28T11:54:20Z
- Parser: tree-sitter-gdscript 6.1.0
- Files: **14**
- Script/nested classes: **0**
- Functions: **130**
- Files with syntax errors: **0**

> Syntax-derived navigation only. Source and tests remain authoritative.
> Call edges are included only for direct calls to functions declared in the same file.

## colony/citizens/citizen_model.gd

- Functions:
  - create_citizen() — line 62
  - normalize_citizen() — line 87; calls _update_summary(), create_citizen()
  - advance_daily_needs() — line 127; calls _apply_supply(), _update_summary()
  - record_work_day() — line 174
  - get_happiness_breakdown() — line 195
  - get_balance_data() — line 214
  - _apply_supply() — line 218
  - _update_summary() — line 223; calls get_happiness_breakdown()
  - _merge_balance() — line 236

## colony/colony_simulation.gd

- Functions:
  - _ready() — line 40; calls start_new_game()
  - start_new_game() — line 50; calls _assign_homes(), _assign_workers(), _set_message()
  - can_spend() — line 71
  - try_spend() — line 78; calls can_spend()
  - can_build() — line 87; calls can_spend()
  - build() — line 94; calls _assign_homes(), _assign_workers(), _set_message(), try_spend()
  - has_building() — line 112
  - get_housing_capacity() — line 119
  - get_housed_count() — line 128
  - get_employed_count() — line 136
  - get_average_happiness() — line 144
  - get_population_summary() — line 153; calls get_average_happiness(), get_employed_count(), get_housed_count(), get_housing_capacity()
  - get_citizen_inspection() — line 160
  - advance_day() — line 169; calls _set_message()
  - get_snapshot() — line 234
  - restore_snapshot() — line 244; calls _assign_homes(), _assign_workers(), _set_message()
  - _assign_homes() — line 292
  - _assign_workers() — line 326
  - _set_message() — line 359

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
  - set_world() — line 12
  - _draw() — line 19; calls _draw_building(), _draw_citizens(), _draw_rocket()
  - _draw_building() — line 50
  - _draw_citizens() — line 76
  - _draw_rocket() — line 84

## rendering/world_view.gd

- Functions:
  - _ready() — line 41
  - set_world() — line 48; calls focus_city()
  - set_launch_capability_unlocked() — line 74; calls _start_home_planet_reveal()
  - reveal_home_planet() — line 99; calls set_launch_capability_unlocked()
  - is_planetary_zoom_unlocked() — line 103
  - is_full_planet_visible() — line 107
  - get_camera_zoom() — line 111
  - get_target_camera_zoom() — line 115
  - get_target_camera_focus() — line 119
  - is_camera_transition_active() — line 123
  - focus_city() — line 127; calls _clamp_camera_focus()
  - get_camera_scale_name() — line 136
  - _start_home_planet_reveal() — line 144
  - _process() — line 155; calls get_camera_scale_name()
  - _gui_input() — line 182; calls _apply_zoom(), _clamp_camera_focus()
  - _apply_zoom() — line 239; calls _clamp_camera_focus()
  - _clamp_camera_focus() — line 247
  - _draw() — line 260; calls _draw_buildings(), _draw_citizens(), _draw_city_surface(), _draw_home_planet(), _draw_rocket_marker(), _draw_sky(), _draw_view_label()
  - _draw_sky() — line 281
  - _draw_city_surface() — line 300
  - _draw_home_planet() — line 310
  - _draw_buildings() — line 319; calls _building_color(), _draw_building()
  - _draw_building() — line 339; calls _building_color()
  - _building_color() — line 370
  - _draw_citizens() — line 387
  - _draw_rocket_marker() — line 410
  - _draw_view_label() — line 423; calls get_camera_scale_name()

## saves/save_service.gd

- Functions:
  - save_game() — line 7
  - load_game() — line 32

## tests/gameplay_smoke.gd

- Functions:
  - _initialize() — line 10; calls _check()
  - _check() — line 115

## tests/native_extension_smoke.gd

- Functions:
  - _initialize() — line 8
  - _run_native_probe() — line 12

## ui/screens/boot_screen.gd

- Functions:
  - _ready() — line 15; calls _check_native_runtime(), _update_touch_count()
  - _check_native_runtime() — line 27
  - _on_test_touch_pressed() — line 54; calls _update_touch_count()
  - _on_test_external_script_pressed() — line 60
  - _on_external_probe_finished() — line 65
  - _on_install_sample_mod_pressed() — line 73
  - _on_package_finished() — line 78
  - _on_disable_sample_mod_pressed() — line 83
  - _on_application_paused() — line 88
  - _on_application_resumed() — line 92
  - _on_close_pressed() — line 96
  - _update_touch_count() — line 100

## ui/screens/game_screen.gd

- Functions:
  - _ready() — line 17; calls _build_interface(), _build_world_view(), _refresh()
  - _build_world_view() — line 31
  - _build_interface() — line 38; calls _make_button(), _make_panel_style()
  - _make_panel_style() — line 121
  - _make_button() — line 134
  - _on_build_pressed() — line 144; calls _refresh()
  - _on_city_pressed() — line 149
  - _on_save_pressed() — line 153; calls _on_message_changed()
  - _on_load_pressed() — line 158; calls _on_message_changed(), _refresh()
  - _on_diagnostics_pressed() — line 172
  - _close_diagnostics_overlay() — line 180
  - _on_camera_scale_changed() — line 186; calls _refresh()
  - _on_message_changed() — line 190
  - _on_application_paused() — line 195; calls _on_message_changed()
  - _on_application_resumed() — line 199; calls _on_message_changed()
  - _refresh() — line 203

## vehicles/rocket_flight.gd

- Functions:
  - _process() — line 30; calls advance_simulation()
  - construct() — line 40; calls _emit_message()
  - can_launch() — line 52
  - can_return() — line 56
  - is_unbuilt() — line 60
  - is_on_surface() — line 64
  - is_in_orbit() — line 68
  - has_rocket() — line 72
  - is_flight_active() — line 76
  - launch() — line 80; calls _emit_message(), can_launch()
  - begin_return() — line 91; calls _emit_message(), can_return()
  - advance_simulation() — line 102; calls _step_physics()
  - get_altitude() — line 110
  - get_orbital_speed() — line 114
  - get_circular_speed() — line 118
  - get_orbital_period_seconds() — line 122
  - get_status_text() — line 127; calls get_altitude(), get_orbital_period_seconds(), get_orbital_speed()
  - get_snapshot() — line 144
  - restore_snapshot() — line 156
  - _step_physics() — line 178; calls _emit_message(), get_circular_speed()
  - _emit_message() — line 210

## world/home_world_definition.gd

- Functions: none detected

## CodeGraphContext (supported native/script languages)
# CodeGraphContext Code Map

- Commit: 82873e2601c5d4f0da934a1ec4ae3d8343c65afc
- Generated: 2026-11-28T11:54:17Z
- Indexer: CodeGraphContext 0.5.1

## Index statistics
Resolving context...
Initializing services and database connection...
Services initialized.
📊 Statistics for: /home/runner/work/my-engine-/my-engine-

┏━━━━━━━━━━━━━━━━━━┳━━━━━━━┓
┃ Metric           ┃ Count ┃
┡━━━━━━━━━━━━━━━━━━╇━━━━━━━┩
│ Files            │    39 │
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
