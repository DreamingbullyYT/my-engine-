extends Control

const WorldViewScript = preload("res://rendering/world_view.gd")
const DiagnosticsScene = preload("res://ui/screens/boot_screen.tscn")

var _world_view
var _diagnostics_overlay

var _day_label: Label
var _resource_label: Label
var _population_label: Label
var _rocket_label: Label
var _message_label: Label
var _building_buttons: Dictionary = {}
var _build_rocket_button: Button
var _launch_button: Button
var _return_button: Button
var _city_button: Button


func _ready() -> void:
	_build_world_view()
	_build_interface()

	_world_view.camera_scale_changed.connect(_on_camera_scale_changed)
	_world_view.camera_transition_finished.connect(_refresh)
	ColonySimulation.changed.connect(_refresh)
	ColonySimulation.rocket_capability_unlocked.connect(_on_rocket_capability_unlocked)
	ColonySimulation.message_changed.connect(_on_message_changed)
	RocketFlight.changed.connect(_refresh)
	RocketFlight.message_changed.connect(_on_message_changed)
	AppLifecycle.application_paused.connect(_on_application_paused)
	AppLifecycle.application_resumed.connect(_on_application_resumed)

	_world_view.set_launch_capability_unlocked(ColonySimulation.launch_capability_unlocked, false)
	_refresh()


func _build_world_view() -> void:
	_world_view = WorldViewScript.new()
	_world_view.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	_world_view.mouse_filter = Control.MOUSE_FILTER_STOP
	add_child(_world_view)


func _build_interface() -> void:
	var top_panel := PanelContainer.new()
	top_panel.anchor_left = 0.02
	top_panel.anchor_top = 0.015
	top_panel.anchor_right = 0.98
	top_panel.anchor_bottom = 0.19
	top_panel.offset_left = 0.0
	top_panel.offset_top = 0.0
	top_panel.offset_right = 0.0
	top_panel.offset_bottom = 0.0
	top_panel.add_theme_stylebox_override("panel", _make_panel_style())
	add_child(top_panel)

	var top_content := VBoxContainer.new()
	top_content.add_theme_constant_override("separation", 3)
	top_panel.add_child(top_content)

	var header := HBoxContainer.new()
	top_content.add_child(header)
	var title := Label.new()
	title.text = "DREAM COLONY"
	title.add_theme_font_size_override("font_size", 18)
	title.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	header.add_child(title)
	_day_label = Label.new()
	_day_label.add_theme_font_size_override("font_size", 14)
	header.add_child(_day_label)

	_resource_label = Label.new()
	_resource_label.add_theme_font_size_override("font_size", 13)
	top_content.add_child(_resource_label)

	_population_label = Label.new()
	_population_label.add_theme_font_size_override("font_size", 12)
	top_content.add_child(_population_label)

	_rocket_label = Label.new()
	_rocket_label.add_theme_font_size_override("font_size", 12)
	top_content.add_child(_rocket_label)

	var bottom_panel := PanelContainer.new()
	bottom_panel.anchor_left = 0.02
	bottom_panel.anchor_top = 0.68
	bottom_panel.anchor_right = 0.98
	bottom_panel.anchor_bottom = 0.985
	bottom_panel.add_theme_stylebox_override("panel", _make_panel_style())
	add_child(bottom_panel)

	var bottom_content := VBoxContainer.new()
	bottom_content.add_theme_constant_override("separation", 4)
	bottom_panel.add_child(bottom_content)

	var build_grid := GridContainer.new()
	build_grid.columns = 3
	build_grid.add_theme_constant_override("h_separation", 5)
	build_grid.add_theme_constant_override("v_separation", 4)
	bottom_content.add_child(build_grid)

	var building_labels := {
		"habitat": "Habitat · 18 M",
		"solar": "Solar · 14 M",
		"farm": "Farm · 16 M",
		"mine": "Mine · 20 M",
		"launchpad": "Launch pad · 30 M"
	}
	for building_type in ["habitat", "solar", "farm", "mine", "launchpad"]:
		var button := _make_button(String(building_labels[building_type]), _on_build_pressed.bind(building_type), 38)
		_building_buttons[building_type] = button
		build_grid.add_child(button)

	_build_rocket_button = _make_button("Build rocket · 20 M / 20 F", _on_build_rocket_pressed, 38)
	build_grid.add_child(_build_rocket_button)

	var action_row := HBoxContainer.new()
	action_row.add_theme_constant_override("separation", 4)
	bottom_content.add_child(action_row)

	_launch_button = _make_button("Launch", _on_launch_pressed, 36)
	_return_button = _make_button("Return", _on_return_pressed, 36)
	_city_button = _make_button("City", _on_city_pressed, 36)
	var save_button := _make_button("Save", _on_save_pressed, 36)
	var load_button := _make_button("Load", _on_load_pressed, 36)
	var diagnostics_button := _make_button("Diagnostics", _on_diagnostics_pressed, 36)
	for button in [_launch_button, _return_button, _city_button, save_button, load_button, diagnostics_button]:
		action_row.add_child(button)

	_message_label = Label.new()
	_message_label.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	_message_label.add_theme_font_size_override("font_size", 11)
	bottom_content.add_child(_message_label)


func _make_panel_style() -> StyleBoxFlat:
	var style := StyleBoxFlat.new()
	style.bg_color = Color(0.035, 0.055, 0.075, 0.88)
	style.border_color = Color(0.22, 0.32, 0.37, 0.85)
	style.set_border_width_all(1)
	style.set_corner_radius_all(8)
	style.content_margin_left = 8.0
	style.content_margin_top = 5.0
	style.content_margin_right = 8.0
	style.content_margin_bottom = 5.0
	return style


func _make_button(label: String, action: Callable, height: float) -> Button:
	var button := Button.new()
	button.text = label
	button.custom_minimum_size = Vector2(0.0, height)
	button.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	button.add_theme_font_size_override("font_size", 12)
	button.pressed.connect(action)
	return button


func _on_build_pressed(building_type: String) -> void:
	ColonySimulation.build(building_type)
	_refresh()


func _on_build_rocket_pressed() -> void:
	if not ColonySimulation.launch_capability_unlocked:
		_on_message_changed("Build the launch pad first.")
		return
	if not ColonySimulation.try_spend({"metal": 20, "fuel": 20}):
		_on_message_changed("Rocket construction requires 20 metal and 20 fuel.")
		return
	if not RocketFlight.construct():
		_on_message_changed("A rocket already exists.")
	_refresh()


func _on_launch_pressed() -> void:
	if not RocketFlight.launch():
		_on_message_changed("Build a rocket before launching.")
	_refresh()


func _on_return_pressed() -> void:
	if not RocketFlight.begin_return():
		_on_message_changed("A stable orbit is required before returning.")
	_refresh()


func _on_city_pressed() -> void:
	_world_view.focus_city()


func _on_save_pressed() -> void:
	var saved := SaveService.save_game(ColonySimulation.get_snapshot(), RocketFlight.get_snapshot())
	_on_message_changed("Game saved to the local slot." if saved else "Save failed. Check the log.")


func _on_load_pressed() -> void:
	var result: Dictionary = SaveService.load_game()
	if not bool(result.get("ok", false)):
		_on_message_changed(String(result.get("error", "Could not load save.")))
		return
	var payload: Dictionary = result.get("data", {})
	var colony_data: Dictionary = payload.get("colony", {})
	var rocket_data: Dictionary = payload.get("rocket", {})
	if not ColonySimulation.restore_snapshot(colony_data):
		_on_message_changed("Colony save data is invalid.")
		return
	if not RocketFlight.restore_snapshot(rocket_data):
		_on_message_changed("Rocket save data is invalid.")
		return
	_world_view.set_launch_capability_unlocked(ColonySimulation.launch_capability_unlocked, false)
	_on_message_changed("Save loaded.")
	_refresh()


func _on_diagnostics_pressed() -> void:
	if is_instance_valid(_diagnostics_overlay):
		return
	_diagnostics_overlay = DiagnosticsScene.instantiate()
	_diagnostics_overlay.close_requested.connect(_close_diagnostics_overlay)
	add_child(_diagnostics_overlay)


func _close_diagnostics_overlay() -> void:
	if is_instance_valid(_diagnostics_overlay):
		_diagnostics_overlay.queue_free()
		_diagnostics_overlay = null


func _on_rocket_capability_unlocked() -> void:
	_world_view.reveal_home_planet()
	_on_message_changed("Rocket launch capability unlocked. Revealing the home planet.")


func _on_camera_scale_changed(scale_name: String) -> void:
	_refresh()


func _on_message_changed(message: String) -> void:
	if is_instance_valid(_message_label):
		_message_label.text = message


func _on_application_paused() -> void:
	_on_message_changed("Application paused. Simulation time stops while the app is closed.")


func _on_application_resumed() -> void:
	_on_message_changed("Welcome back. Colony simulation resumed.")


func _refresh() -> void:
	if not is_instance_valid(_world_view):
		return
	_day_label.text = "DAY %d" % ColonySimulation.day
	var stock: Dictionary = ColonySimulation.resources
	_resource_label.text = "METAL %d   FOOD %d   ENERGY %d   FUEL %d" % [
		int(stock.get("metal", 0)),
		int(stock.get("food", 0)),
		int(stock.get("energy", 0)),
		int(stock.get("fuel", 0))
	]
	_population_label.text = ColonySimulation.get_population_summary()
	_rocket_label.text = RocketFlight.get_status_text()
	_world_view.set_world(ColonySimulation.buildings, ColonySimulation.citizens, RocketFlight.has_rocket(), RocketFlight.is_flight_active(), RocketFlight.is_on_surface(), RocketFlight.position)

	for building_type in _building_buttons:
		var button: Button = _building_buttons[building_type]
		button.disabled = not ColonySimulation.can_build(String(building_type))

	_build_rocket_button.disabled = not (
		RocketFlight.is_unbuilt()
		and ColonySimulation.launch_capability_unlocked
		and ColonySimulation.can_spend({"metal": 20, "fuel": 20})
	)
	_launch_button.disabled = not RocketFlight.can_launch() or _world_view.is_camera_transition_active()
	_return_button.disabled = not RocketFlight.can_return()
