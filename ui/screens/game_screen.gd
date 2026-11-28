extends Control

const PlanetViewScript = preload("res://rendering/planet_view.gd")

var _planet_view: Control
var _day_label: Label
var _resource_label: Label
var _population_label: Label
var _rocket_label: Label
var _message_label: Label
var _building_buttons: Dictionary = {}
var _build_rocket_button: Button
var _launch_button: Button
var _return_button: Button


func _ready() -> void:
	_build_interface()
	ColonySimulation.changed.connect(_refresh)
	ColonySimulation.message_changed.connect(_on_message_changed)
	RocketFlight.changed.connect(_refresh)
	RocketFlight.message_changed.connect(_on_message_changed)
	AppLifecycle.application_paused.connect(_on_application_paused)
	AppLifecycle.application_resumed.connect(_on_application_resumed)
	_refresh()


func _build_interface() -> void:
	var background := ColorRect.new()
	background.color = Color(0.025, 0.04, 0.065, 1.0)
	background.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	add_child(background)

	var margin := MarginContainer.new()
	margin.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	margin.add_theme_constant_override("margin_left", 12)
	margin.add_theme_constant_override("margin_top", 8)
	margin.add_theme_constant_override("margin_right", 12)
	margin.add_theme_constant_override("margin_bottom", 8)
	add_child(margin)

	var layout := VBoxContainer.new()
	layout.add_theme_constant_override("separation", 5)
	margin.add_child(layout)

	var header := HBoxContainer.new()
	layout.add_child(header)
	var title := Label.new()
	title.text = "DREAM COLONY"
	title.add_theme_font_size_override("font_size", 21)
	title.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	header.add_child(title)
	_day_label = Label.new()
	_day_label.add_theme_font_size_override("font_size", 15)
	header.add_child(_day_label)

	_resource_label = Label.new()
	_resource_label.add_theme_font_size_override("font_size", 14)
	layout.add_child(_resource_label)

	var world_panel := PanelContainer.new()
	world_panel.size_flags_vertical = Control.SIZE_EXPAND_FILL
	world_panel.custom_minimum_size = Vector2(0.0, 170.0)
	var panel_style := StyleBoxFlat.new()
	panel_style.bg_color = Color(0.045, 0.07, 0.105, 1.0)
	panel_style.set_corner_radius_all(8)
	world_panel.add_theme_stylebox_override("panel", panel_style)
	layout.add_child(world_panel)

	_planet_view = PlanetViewScript.new()
	_planet_view.custom_minimum_size = Vector2(0.0, 170.0)
	_planet_view.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	_planet_view.size_flags_vertical = Control.SIZE_EXPAND_FILL
	world_panel.add_child(_planet_view)

	_population_label = Label.new()
	_population_label.add_theme_font_size_override("font_size", 13)
	layout.add_child(_population_label)

	_rocket_label = Label.new()
	_rocket_label.add_theme_font_size_override("font_size", 13)
	layout.add_child(_rocket_label)

	var build_grid := GridContainer.new()
	build_grid.columns = 3
	build_grid.add_theme_constant_override("h_separation", 6)
	build_grid.add_theme_constant_override("v_separation", 5)
	layout.add_child(build_grid)

	var building_labels := {
		"habitat": "Habitat · 18 M",
		"solar": "Solar · 14 M",
		"farm": "Farm · 16 M",
		"mine": "Mine · 20 M",
		"launchpad": "Launch pad · 30 M"
	}
	for building_type in ["habitat", "solar", "farm", "mine", "launchpad"]:
		var button := _make_button(String(building_labels[building_type]), _on_build_pressed.bind(building_type))
		_building_buttons[building_type] = button
		build_grid.add_child(button)

	_build_rocket_button = _make_button("Build rocket · 20 M / 20 F", _on_build_rocket_pressed)
	build_grid.add_child(_build_rocket_button)

	var flight_row := HBoxContainer.new()
	flight_row.add_theme_constant_override("separation", 5)
	layout.add_child(flight_row)

	_launch_button = _make_button("Launch", _on_launch_pressed)
	_return_button = _make_button("Return", _on_return_pressed)
	var save_button := _make_button("Save", _on_save_pressed)
	var load_button := _make_button("Load", _on_load_pressed)
	var diagnostics_button := _make_button("Diagnostics", _on_diagnostics_pressed)
	for button in [_launch_button, _return_button, save_button, load_button, diagnostics_button]:
		flight_row.add_child(button)

	_message_label = Label.new()
	_message_label.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	_message_label.add_theme_font_size_override("font_size", 12)
	layout.add_child(_message_label)


func _make_button(label: String, action: Callable) -> Button:
	var button := Button.new()
	button.text = label
	button.custom_minimum_size = Vector2(0.0, 42.0)
	button.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	button.add_theme_font_size_override("font_size", 13)
	button.pressed.connect(action)
	return button


func _on_build_pressed(building_type: String) -> void:
	ColonySimulation.build(building_type)
	_refresh()


func _on_build_rocket_pressed() -> void:
	if not ColonySimulation.has_building("launchpad"):
		_on_message_changed("Build a launch pad first.")
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
	_on_message_changed("Save loaded.")
	_refresh()


func _on_diagnostics_pressed() -> void:
	get_tree().change_scene_to_file("res://ui/screens/boot_screen.tscn")


func _on_message_changed(message: String) -> void:
	if is_instance_valid(_message_label):
		_message_label.text = message


func _on_application_paused() -> void:
	_on_message_changed("Application paused. Simulation time stops while the app is closed.")


func _on_application_resumed() -> void:
	_on_message_changed("Welcome back. Colony simulation resumed.")


func _refresh() -> void:
	if not is_instance_valid(_planet_view):
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
	_planet_view.call("set_world", ColonySimulation.buildings, RocketFlight.state, RocketFlight.position)

	for building_type in _building_buttons:
		var button: Button = _building_buttons[building_type]
		button.disabled = not ColonySimulation.can_build(String(building_type))

	_build_rocket_button.disabled = not (
		RocketFlight.is_unbuilt()
		and ColonySimulation.has_building("launchpad")
		and ColonySimulation.can_spend({"metal": 20, "fuel": 20})
	)
	_launch_button.disabled = not RocketFlight.can_launch()
	_return_button.disabled = not RocketFlight.can_return()
