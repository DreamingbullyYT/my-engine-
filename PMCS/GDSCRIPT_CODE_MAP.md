# GDScript Code Map

- Source commit: 7f7be485889104f57a86159f2014727ae4c50464
- Generated (UTC): 2026-11-28T11:40:03Z
- Parser: tree-sitter-gdscript 6.1.0
- Files: **13**
- Script/nested classes: **0**
- Functions: **122**
- Files with syntax errors: **0**

> Syntax-derived navigation only. Source and tests remain authoritative.
> Call edges are included only for direct calls to functions declared in the same file.

## colony/colony_simulation.gd

- Functions:
  - _ready() — line 65; calls start_new_game()
  - start_new_game() — line 75; calls _assign_workers(), _set_message()
  - can_spend() — line 102
  - try_spend() — line 109; calls can_spend()
  - can_build() — line 118; calls can_spend()
  - build() — line 125; calls _assign_workers(), _set_message(), try_spend()
  - has_building() — line 151
  - get_housing_capacity() — line 158
  - get_employed_count() — line 167
  - get_average_happiness() — line 175
  - advance_day() — line 184; calls _set_message()
  - get_population_summary() — line 239; calls get_average_happiness(), get_employed_count(), get_housing_capacity()
  - get_snapshot() — line 248
  - restore_snapshot() — line 259; calls _assign_workers(), _set_message(), has_building()
  - _assign_workers() — line 298
  - _set_message() — line 328

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
  - _draw_citizens() — line 385
  - _draw_rocket_marker() — line 408
  - _draw_view_label() — line 421; calls get_camera_scale_name()

## saves/save_service.gd

- Functions:
  - save_game() — line 7
  - load_game() — line 33

## tests/gameplay_smoke.gd

- Functions:
  - _initialize() — line 11; calls _check()
  - _check() — line 85

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
  - _ready() — line 21; calls _build_interface(), _build_world_view(), _refresh()
  - _build_world_view() — line 39
  - _build_interface() — line 46; calls _make_button(), _make_panel_style()
  - _make_panel_style() — line 138
  - _make_button() — line 151
  - _on_build_pressed() — line 161; calls _refresh()
  - _on_build_rocket_pressed() — line 166; calls _on_message_changed(), _refresh()
  - _on_launch_pressed() — line 178; calls _on_message_changed(), _refresh()
  - _on_return_pressed() — line 184; calls _on_message_changed(), _refresh()
  - _on_city_pressed() — line 190
  - _on_save_pressed() — line 194; calls _on_message_changed()
  - _on_load_pressed() — line 199; calls _on_message_changed(), _refresh()
  - _on_diagnostics_pressed() — line 218
  - _close_diagnostics_overlay() — line 226
  - _on_rocket_capability_unlocked() — line 232; calls _on_message_changed()
  - _on_camera_scale_changed() — line 237; calls _refresh()
  - _on_message_changed() — line 241
  - _on_application_paused() — line 246; calls _on_message_changed()
  - _on_application_resumed() — line 250; calls _on_message_changed()
  - _refresh() — line 254

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
