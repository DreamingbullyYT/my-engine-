# Dream Engine Code Map

- Source commit: 61d7b897c1d4b11c0e2e421592ef6bf16554d727
- Generated: 2026-11-28T15:12:39Z

## GDScript
# GDScript Code Map

- Source commit: 61d7b897c1d4b11c0e2e421592ef6bf16554d727
- Generated (UTC): 2026-11-28T15:12:39Z
- Parser: tree-sitter-gdscript 6.1.0
- Files: **23**
- Script/nested classes: **0**
- Functions: **249**
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
  - _ready() — line 56; calls start_new_game()
  - start_new_game() — line 66; calls _assign_homes(), _assign_workers(), _set_message()
  - can_spend() — line 87
  - try_spend() — line 94; calls can_spend()
  - can_build() — line 103; calls can_spend()
  - build() — line 110; calls _set_message(), build_at(), get_building_intervals(), get_building_width()
  - get_building_width() — line 123
  - get_building_intervals() — line 130; calls get_building_width()
  - can_place() — line 140; calls get_building_intervals(), get_building_width()
  - build_at() — line 149; calls _assign_homes(), _assign_workers(), _next_building_id(), _set_message(), can_place(), can_spend(), try_spend()
  - find_building() — line 175
  - has_building() — line 182
  - get_housing_capacity() — line 189
  - get_housed_count() — line 198
  - get_employed_count() — line 206
  - get_average_happiness() — line 214
  - get_population_summary() — line 223; calls get_average_happiness(), get_employed_count(), get_housed_count(), get_housing_capacity()
  - get_citizen_inspection() — line 230
  - advance_day() — line 239; calls _set_message()
  - get_snapshot() — line 304
  - restore_snapshot() — line 314; calls _assign_homes(), _assign_workers(), _set_message()
  - _assign_homes() — line 362
  - _assign_workers() — line 396
  - _next_building_id() — line 429; calls find_building()
  - _set_message() — line 436

## colony/placement.gd

- Functions:
  - angle_to_tile() — line 16
  - tile_to_angle() — line 20
  - interval_for() — line 25; calls angle_to_tile()
  - is_free() — line 30
  - snap_center_tile() — line 44
  - nearest_free_center() — line 61; calls is_free()

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

## rendering/city/building_art.gd

- Functions:
  - height_of() — line 37
  - paint() — line 42; calls _paint_farm(), _paint_habitat(), _paint_mine(), _paint_pump(), _paint_solar(), _paint_status_bubble()
  - _paint_habitat() — line 59; calls _habitat_window()
  - _habitat_window() — line 84; calls _hash()
  - _paint_solar() — line 97
  - _paint_farm() — line 124; calls _hash()
  - _paint_pump() — line 148
  - _paint_mine() — line 174; calls _line()
  - _paint_status_bubble() — line 208; calls height_of()
  - _line() — line 221
  - _hash() — line 227

## rendering/city/citizen_walkers.gd

- Functions:
  - sync_citizens() — line 38; calls _spawn()
  - update() — line 69; calls _update_walker()
  - walker_count() — line 75
  - visible_count() — line 79
  - citizen_at() — line 88
  - position_of() — line 104
  - draw() — line 110; calls _draw_figure()
  - _draw_figure() — line 121
  - _spawn() — line 154
  - _update_walker() — line 178; calls _desired_mode(), _go_inside_at(), _wander(), _work_cycle()
  - _desired_mode() — line 196
  - _go_inside_at() — line 204; calls _walk_toward()
  - _work_cycle() — line 211; calls _walk_toward(), _wander()
  - _wander() — line 233; calls _walk_toward()
  - _walk_toward() — line 247

## rendering/city/city_scenery.gd

- Functions:
  - sun_height() — line 27
  - daylight() — line 31; calls sun_height()
  - night_amount() — line 35; calls daylight()
  - draw_sky() — line 39; calls _draw_clouds(), _draw_hills(), _draw_stars(), _draw_sun_and_moon(), daylight(), sun_height()
  - _draw_stars() — line 61; calls _hash()
  - _draw_sun_and_moon() — line 71
  - _draw_hills() — line 88; calls _draw_ridge()
  - _draw_ridge() — line 95
  - _draw_clouds() — line 109; calls _hash()
  - draw_ground() — line 127; calls _ground_details(), _ground_layer(), _ground_plants(), _tile_is_free()
  - _ground_layer() — line 148
  - _ground_details() — line 157; calls _hash()
  - _ground_plants() — line 177; calls _hash()
  - _tile_is_free() — line 193
  - _hash() — line 201

## rendering/city/pixel_painter.gd

- Functions:
  - begin_world() — line 28
  - begin_flat() — line 38
  - set_origin_x() — line 49
  - rect() — line 54; calls _project(), _shaded()
  - poly() — line 65; calls _project_point(), _shaded()
  - outline() — line 73; calls rect()
  - light() — line 81; calls _project()
  - flush_lights() — line 93
  - _shaded() — line 102
  - _project_point() — line 106; calls _project()
  - _project() — line 113

## rendering/planet_view.gd

- Functions:
  - set_world() — line 12
  - _draw() — line 19; calls _draw_building(), _draw_citizens(), _draw_rocket()
  - _draw_building() — line 50
  - _draw_citizens() — line 76
  - _draw_rocket() — line 84

## rendering/world_view.gd

- Functions:
  - _ready() — line 86
  - set_world() — line 95; calls _widths(), focus_city(), is_ghost_valid()
  - set_building_catalog() — line 125; calls _widths()
  - set_launch_capability_unlocked() — line 131; calls _start_home_planet_reveal()
  - reveal_home_planet() — line 156; calls set_launch_capability_unlocked()
  - is_planetary_zoom_unlocked() — line 160
  - is_full_planet_visible() — line 164
  - get_camera_zoom() — line 168
  - get_target_camera_zoom() — line 172
  - get_target_camera_focus() — line 176
  - is_camera_transition_active() — line 180
  - focus_city() — line 184; calls _clamp_camera_focus()
  - get_camera_scale_name() — line 193
  - set_time_of_day() — line 202
  - get_time_of_day() — line 207
  - get_frames_drawn() — line 212
  - select_building() — line 218
  - select_citizen() — line 224
  - clear_selection() — line 230
  - begin_placement() — line 236; calls _intervals(), _widths(), clear_selection(), is_ghost_valid()
  - end_placement() — line 246
  - is_placing() — line 252
  - is_ghost_valid() — line 256; calls _intervals()
  - get_ghost_angle() — line 262
  - _process() — line 270; calls get_camera_scale_name()
  - _start_home_planet_reveal() — line 304
  - _gui_input() — line 317; calls _apply_zoom(), _begin_press(), _handle_drag(), _handle_tap(), _handle_touch(), _pan_by(), _press_is_tap()
  - _handle_touch() — line 357; calls _begin_press(), _handle_tap(), _press_is_tap()
  - _handle_drag() — line 376; calls _apply_zoom(), _pan_by()
  - _pan_by() — line 397; calls _clamp_camera_focus()
  - _begin_press() — line 404
  - _press_is_tap() — line 411
  - _handle_tap() — line 419; calls _building_at(), _intervals(), _planet_mix(), _screen_to_world(), clear_selection(), is_ghost_valid(), is_placing(), select_building(), select_citizen()
  - _building_at() — line 454; calls _widths()
  - _apply_zoom() — line 467; calls _clamp_camera_focus()
  - _clamp_camera_focus() — line 475
  - _planet_mix() — line 487
  - _screen_anchor() — line 493
  - _screen_to_world() — line 497; calls _planet_mix(), _screen_anchor()
  - _widths() — line 502
  - _intervals() — line 509; calls _widths()
  - _draw() — line 520; calls _draw_backdrop(), _draw_buildings(), _draw_ghost(), _draw_home_planet(), _draw_rocket_marker(), _intervals(), _planet_mix(), _screen_anchor()
  - _draw_backdrop() — line 558
  - _draw_home_planet() — line 571
  - _draw_buildings() — line 578; calls _widths()
  - _draw_ghost() — line 607; calls is_ghost_valid()
  - _draw_rocket_marker() — line 627

## saves/save_service.gd

- Functions:
  - save_game() — line 7
  - load_game() — line 32

## tests/city_view_smoke.gd

- Functions:
  - _initialize() — line 14; calls _test_colony_placement(), _test_placement_geometry(), _test_scene(), _test_walkers()
  - _test_placement_geometry() — line 28; calls _check()
  - _test_colony_placement() — line 45; calls _check()
  - _test_walkers() — line 83; calls _check()
  - _test_scene() — line 127; calls _check()
  - _check() — line 201

## tests/gameplay_smoke.gd

- Functions:
  - _initialize() — line 10; calls _check()
  - _check() — line 114

## tests/native_extension_smoke.gd

- Functions:
  - _initialize() — line 8
  - _run_native_probe() — line 12

## ui/game_theme.gd

- Functions:
  - panel_style() — line 19
  - button_style() — line 35
  - build() — line 48; calls button_style(), panel_style()

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
  - _ready() — line 51; calls _apply_safe_area(), _build_bottom_bar(), _build_info_panel(), _build_menu_overlay(), _build_toast(), _build_top_bar(), _build_world_view(), _refresh()
  - _build_world_view() — line 78
  - _build_top_bar() — line 85; calls _make_chip(), _make_icon_button()
  - _make_chip() — line 112
  - _make_icon_button() — line 126
  - _make_text_button() — line 138
  - _build_toast() — line 147
  - _build_info_panel() — line 169; calls _make_icon_button()
  - _build_bottom_bar() — line 196; calls _make_text_button()
  - _build_menu_overlay() — line 243; calls _make_text_button()
  - _apply_safe_area() — line 282; calls _read_safe_insets()
  - _read_safe_insets() — line 303
  - _on_card_pressed() — line 323; calls _cancel_placement(), _show_toast(), _start_placement()
  - _start_placement() — line 335; calls _hide_info()
  - _cancel_placement() — line 347
  - _confirm_placement() — line 356; calls _cancel_placement()
  - _on_placement_changed() — line 366
  - _on_building_tapped() — line 381; calls _update_info()
  - _on_citizen_tapped() — line 387; calls _update_info()
  - _hide_info() — line 393
  - _update_info() — line 401; calls _show_building_info(), _show_citizen_info()
  - _show_building_info() — line 411; calls _hide_info()
  - _show_citizen_info() — line 437; calls _hide_info()
  - _open_menu() — line 462
  - _close_menu() — line 466
  - _on_menu_dim_input() — line 470; calls _close_menu()
  - _on_center_pressed() — line 477; calls _close_menu()
  - _on_save_pressed() — line 482; calls _close_menu(), _show_toast()
  - _on_load_pressed() — line 488; calls _cancel_placement(), _close_menu(), _hide_info(), _refresh(), _show_toast()
  - _on_diagnostics_pressed() — line 507; calls _close_menu()
  - _close_diagnostics_overlay() — line 516
  - _on_message_changed() — line 522; calls _show_toast()
  - _show_toast() — line 529
  - _on_application_paused() — line 537; calls _show_toast()
  - _on_application_resumed() — line 541; calls _show_toast()
  - _refresh() — line 547; calls _on_placement_changed(), _update_info()

## ui/widgets/building_card.gd

- Functions:
  - setup() — line 28
  - set_affordable() — line 35
  - set_active() — line 42
  - _draw() — line 49; calls _draw_metal_icon()
  - _draw_metal_icon() — line 79

## ui/widgets/pixel_icon.gd

- Functions:
  - _draw() — line 129

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

- Commit: 61d7b897c1d4b11c0e2e421592ef6bf16554d727
- Generated: 2026-11-28T15:12:37Z
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
