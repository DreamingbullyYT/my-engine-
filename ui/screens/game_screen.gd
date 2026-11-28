extends Control

## Main game screen: resource bar on top, build cards on the bottom, tap-to-place
## construction, and a small info panel for tapped buildings and citizens.
## It only reads colony state and calls colony use-cases; it owns no game rules.

const GameTheme = preload("res://ui/game_theme.gd")
const WorldViewScript = preload("res://rendering/world_view.gd")
const DiagnosticsScene = preload("res://ui/screens/boot_screen.tscn")
const PixelIconScript = preload("res://ui/widgets/pixel_icon.gd")
const BuildingCardScript = preload("res://ui/widgets/building_card.gd")

const BUILD_ORDER: Array = ["habitat", "solar", "farm", "water_pump", "mine"]
const TOP_BAR_HEIGHT := 48.0
const BOTTOM_BAR_HEIGHT := 120.0
const SIDE_MARGIN := 8.0
const INFO_WIDTH := 300.0
const TOAST_SECONDS := 3.5
const LOW_STOCK := 15

var _world_view
var _diagnostics_overlay

var _top_bar: PanelContainer
var _bottom_bar: PanelContainer
var _info_panel: PanelContainer
var _toast_panel: PanelContainer
var _menu_overlay: Control

var _stock_labels: Dictionary = {}
var _people_label: Label
var _mood_label: Label
var _day_label: Label
var _toast_label: Label
var _toast_timer: Timer
var _info_title: Label
var _info_body: Label
var _menu_row: ScrollContainer
var _placement_row: HBoxContainer
var _placement_title: Label
var _placement_hint: Label
var _confirm_button: Button
var _cards: Dictionary = {}

var _placing_type: String = ""
var _info_kind: String = ""
var _info_id: String = ""
var _insets: Vector4 = Vector4.ZERO


func _ready() -> void:
	theme = GameTheme.build()
	_build_world_view()
	_build_top_bar()
	_build_toast()
	_build_info_panel()
	_build_bottom_bar()
	_build_menu_overlay()

	_world_view.set_building_catalog(ColonySimulation.BUILDING_TYPES)
	_world_view.building_tapped.connect(_on_building_tapped)
	_world_view.citizen_tapped.connect(_on_citizen_tapped)
	_world_view.empty_tapped.connect(_hide_info)
	_world_view.placement_changed.connect(_on_placement_changed)
	_world_view.placement_confirm_requested.connect(_confirm_placement)
	ColonySimulation.changed.connect(_refresh)
	ColonySimulation.message_changed.connect(_on_message_changed)
	AppLifecycle.application_paused.connect(_on_application_paused)
	AppLifecycle.application_resumed.connect(_on_application_resumed)
	get_viewport().size_changed.connect(_apply_safe_area)

	_apply_safe_area()
	_refresh()


# --- Construction of the interface ------------------------------------------

func _build_world_view() -> void:
	_world_view = WorldViewScript.new()
	_world_view.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	_world_view.mouse_filter = Control.MOUSE_FILTER_STOP
	add_child(_world_view)


func _build_top_bar() -> void:
	_top_bar = PanelContainer.new()
	_top_bar.set_anchors_preset(Control.PRESET_TOP_WIDE)
	add_child(_top_bar)

	var row := HBoxContainer.new()
	row.add_theme_constant_override("separation", 16)
	_top_bar.add_child(row)

	for kind in ["metal", "food", "water", "energy"]:
		_stock_labels[kind] = _make_chip(row, kind)
	_people_label = _make_chip(row, "people")
	_mood_label = _make_chip(row, "happy")

	var spacer := Control.new()
	spacer.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	row.add_child(spacer)

	_day_label = Label.new()
	_day_label.add_theme_font_size_override("font_size", 17)
	_day_label.add_theme_color_override("font_color", GameTheme.ACCENT)
	row.add_child(_day_label)

	var menu_button := _make_icon_button("menu", _open_menu)
	row.add_child(menu_button)


func _make_chip(parent: Control, kind: String) -> Label:
	var box := HBoxContainer.new()
	box.add_theme_constant_override("separation", 5)
	parent.add_child(box)
	var icon = PixelIconScript.new()
	icon.kind = kind
	box.add_child(icon)
	var label := Label.new()
	label.add_theme_font_size_override("font_size", 17)
	label.custom_minimum_size = Vector2(34.0, 0.0)
	box.add_child(label)
	return label


func _make_icon_button(kind: String, action: Callable) -> Button:
	var button := Button.new()
	button.custom_minimum_size = Vector2(GameTheme.MIN_TOUCH, 38.0)
	button.focus_mode = Control.FOCUS_NONE
	button.pressed.connect(action)
	var icon = PixelIconScript.new()
	icon.kind = kind
	icon.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	button.add_child(icon)
	return button


func _make_text_button(label: String, action: Callable, width: float = 0.0) -> Button:
	var button := Button.new()
	button.text = label
	button.custom_minimum_size = Vector2(width, GameTheme.MIN_TOUCH)
	button.focus_mode = Control.FOCUS_NONE
	button.pressed.connect(action)
	return button


func _build_toast() -> void:
	_toast_panel = PanelContainer.new()
	_toast_panel.set_anchors_preset(Control.PRESET_CENTER_TOP)
	_toast_panel.grow_horizontal = Control.GROW_DIRECTION_BOTH
	_toast_panel.mouse_filter = Control.MOUSE_FILTER_IGNORE
	_toast_panel.visible = false
	add_child(_toast_panel)

	_toast_label = Label.new()
	_toast_label.add_theme_font_size_override("font_size", 14)
	_toast_label.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	_toast_label.custom_minimum_size = Vector2(360.0, 0.0)
	_toast_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	_toast_panel.add_child(_toast_label)

	_toast_timer = Timer.new()
	_toast_timer.one_shot = true
	_toast_timer.wait_time = TOAST_SECONDS
	_toast_timer.timeout.connect(func() -> void: _toast_panel.visible = false)
	add_child(_toast_timer)


func _build_info_panel() -> void:
	_info_panel = PanelContainer.new()
	_info_panel.set_anchors_preset(Control.PRESET_BOTTOM_LEFT)
	_info_panel.grow_vertical = Control.GROW_DIRECTION_BEGIN
	_info_panel.visible = false
	add_child(_info_panel)

	var column := VBoxContainer.new()
	column.add_theme_constant_override("separation", 4)
	column.custom_minimum_size = Vector2(INFO_WIDTH - 20.0, 0.0)
	_info_panel.add_child(column)

	var header := HBoxContainer.new()
	column.add_child(header)
	_info_title = Label.new()
	_info_title.add_theme_font_size_override("font_size", 18)
	_info_title.add_theme_color_override("font_color", GameTheme.ACCENT)
	_info_title.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	header.add_child(_info_title)
	header.add_child(_make_icon_button("close", _hide_info))

	_info_body = Label.new()
	_info_body.add_theme_font_size_override("font_size", 14)
	_info_body.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	column.add_child(_info_body)


func _build_bottom_bar() -> void:
	_bottom_bar = PanelContainer.new()
	_bottom_bar.set_anchors_preset(Control.PRESET_BOTTOM_WIDE)
	add_child(_bottom_bar)

	_menu_row = ScrollContainer.new()
	_menu_row.horizontal_scroll_mode = ScrollContainer.SCROLL_MODE_AUTO
	_menu_row.vertical_scroll_mode = ScrollContainer.SCROLL_MODE_DISABLED
	_menu_row.scroll_deadzone = 12
	_bottom_bar.add_child(_menu_row)

	var cards := HBoxContainer.new()
	cards.add_theme_constant_override("separation", 8)
	cards.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	cards.alignment = BoxContainer.ALIGNMENT_CENTER
	_menu_row.add_child(cards)
	for building_type in BUILD_ORDER:
		var definition: Dictionary = ColonySimulation.BUILDING_TYPES[building_type]
		var card = BuildingCardScript.new()
		card.setup(building_type, String(definition.get("label", building_type)), int(definition.get("cost", {}).get("metal", 0)))
		card.pressed.connect(_on_card_pressed.bind(building_type))
		cards.add_child(card)
		_cards[building_type] = card

	_placement_row = HBoxContainer.new()
	_placement_row.add_theme_constant_override("separation", 12)
	_placement_row.visible = false
	_bottom_bar.add_child(_placement_row)

	_placement_row.add_child(_make_text_button("Cancel", _cancel_placement, 110.0))
	var text_column := VBoxContainer.new()
	text_column.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	text_column.alignment = BoxContainer.ALIGNMENT_CENTER
	_placement_row.add_child(text_column)
	_placement_title = Label.new()
	_placement_title.add_theme_font_size_override("font_size", 18)
	_placement_title.add_theme_color_override("font_color", GameTheme.ACCENT)
	text_column.add_child(_placement_title)
	_placement_hint = Label.new()
	_placement_hint.add_theme_font_size_override("font_size", 13)
	_placement_hint.add_theme_color_override("font_color", GameTheme.TEXT_DIM)
	_placement_hint.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	text_column.add_child(_placement_hint)
	_confirm_button = _make_text_button("Build", _confirm_placement, 130.0)
	_placement_row.add_child(_confirm_button)


func _build_menu_overlay() -> void:
	_menu_overlay = Control.new()
	_menu_overlay.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	_menu_overlay.visible = false
	add_child(_menu_overlay)

	var dim := ColorRect.new()
	dim.color = Color(0.0, 0.0, 0.0, 0.55)
	dim.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	dim.gui_input.connect(_on_menu_dim_input)
	_menu_overlay.add_child(dim)

	var center := CenterContainer.new()
	center.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	center.mouse_filter = Control.MOUSE_FILTER_IGNORE
	_menu_overlay.add_child(center)

	var panel := PanelContainer.new()
	center.add_child(panel)
	var column := VBoxContainer.new()
	column.add_theme_constant_override("separation", 8)
	column.custom_minimum_size = Vector2(250.0, 0.0)
	panel.add_child(column)

	var title := Label.new()
	title.text = "Menu"
	title.add_theme_font_size_override("font_size", 20)
	title.add_theme_color_override("font_color", GameTheme.ACCENT)
	title.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	column.add_child(title)
	column.add_child(_make_text_button("Center city", _on_center_pressed))
	column.add_child(_make_text_button("Save game", _on_save_pressed))
	column.add_child(_make_text_button("Load game", _on_load_pressed))
	column.add_child(_make_text_button("Diagnostics", _on_diagnostics_pressed))
	column.add_child(_make_text_button("Close", _close_menu))


# --- Layout --------------------------------------------------------------------

func _apply_safe_area() -> void:
	_insets = _read_safe_insets()
	_top_bar.offset_left = _insets.x + SIDE_MARGIN
	_top_bar.offset_right = -(_insets.z + SIDE_MARGIN)
	_top_bar.offset_top = _insets.y + 6.0
	_top_bar.offset_bottom = _top_bar.offset_top + TOP_BAR_HEIGHT

	_bottom_bar.offset_left = _insets.x + SIDE_MARGIN
	_bottom_bar.offset_right = -(_insets.z + SIDE_MARGIN)
	_bottom_bar.offset_bottom = -(_insets.w + 6.0)
	_bottom_bar.offset_top = _bottom_bar.offset_bottom - BOTTOM_BAR_HEIGHT

	_info_panel.offset_left = _insets.x + SIDE_MARGIN
	_info_panel.offset_right = _info_panel.offset_left + INFO_WIDTH
	_info_panel.offset_bottom = _bottom_bar.offset_top - 8.0
	_info_panel.offset_top = _info_panel.offset_bottom - 10.0

	_toast_panel.offset_top = _top_bar.offset_bottom + 8.0


## Display cut-outs and rounded corners, converted to viewport units.
func _read_safe_insets() -> Vector4:
	var window_size := Vector2(DisplayServer.window_get_size())
	var screen_size := Vector2(DisplayServer.screen_get_size())
	var safe := Rect2(DisplayServer.get_display_safe_area())
	if window_size.x <= 0.0 or screen_size.x <= 0.0 or safe.size.x <= 0.0 or safe.size.y <= 0.0:
		return Vector4.ZERO
	var ratio := get_viewport_rect().size.x / window_size.x
	var raw := Vector4(
		safe.position.x,
		safe.position.y,
		screen_size.x - safe.end.x,
		screen_size.y - safe.end.y)
	var clamped := Vector4(
		clampf(raw.x * ratio, 0.0, 120.0), clampf(raw.y * ratio, 0.0, 120.0),
		clampf(raw.z * ratio, 0.0, 120.0), clampf(raw.w * ratio, 0.0, 120.0))
	return clamped


# --- Building placement -------------------------------------------------------

func _on_card_pressed(building_type: String) -> void:
	if _placing_type == building_type:
		_cancel_placement()
		return
	var definition: Dictionary = ColonySimulation.BUILDING_TYPES[building_type]
	if not ColonySimulation.can_build(building_type):
		_show_toast("Not enough metal for %s (needs %d)." % [
			String(definition.get("label", building_type)), int(definition.get("cost", {}).get("metal", 0))])
		return
	_start_placement(building_type)


func _start_placement(building_type: String) -> void:
	_hide_info()
	_placing_type = building_type
	var definition: Dictionary = ColonySimulation.BUILDING_TYPES[building_type]
	_placement_title.text = "Place %s" % String(definition.get("label", building_type))
	_world_view.begin_placement(building_type)
	_menu_row.visible = false
	_placement_row.visible = true
	for type_name in _cards:
		_cards[type_name].set_active(type_name == building_type)


func _cancel_placement() -> void:
	_placing_type = ""
	_world_view.end_placement()
	_placement_row.visible = false
	_menu_row.visible = true
	for type_name in _cards:
		_cards[type_name].set_active(false)


func _confirm_placement() -> void:
	if _placing_type.is_empty():
		return
	var angle: float = _world_view.get_ghost_angle()
	if is_nan(angle):
		return
	if ColonySimulation.build_at(_placing_type, angle):
		_cancel_placement()


func _on_placement_changed(valid: bool) -> void:
	if _placing_type.is_empty():
		return
	var affordable := ColonySimulation.can_build(_placing_type)
	_confirm_button.disabled = not valid or not affordable
	if not affordable:
		_placement_hint.text = "Not enough metal for this building yet."
	elif valid:
		_placement_hint.text = "Tap the ground to move it. Tap the building again, or press Build."
	else:
		_placement_hint.text = "That spot is blocked. Tap somewhere free."


# --- Info panel ------------------------------------------------------------------

func _on_building_tapped(building_id: String) -> void:
	_info_kind = "building"
	_info_id = building_id
	_update_info()


func _on_citizen_tapped(citizen_id: String) -> void:
	_info_kind = "citizen"
	_info_id = citizen_id
	_update_info()


func _hide_info() -> void:
	_info_kind = ""
	_info_id = ""
	_info_panel.visible = false
	if is_instance_valid(_world_view):
		_world_view.clear_selection()


func _update_info() -> void:
	match _info_kind:
		"building":
			_show_building_info()
		"citizen":
			_show_citizen_info()
		_:
			_info_panel.visible = false


func _show_building_info() -> void:
	var building: Dictionary = ColonySimulation.find_building(_info_id)
	if building.is_empty():
		_hide_info()
		return
	var building_type := String(building.get("type", ""))
	var definition: Dictionary = ColonySimulation.BUILDING_TYPES.get(building_type, {})
	var lines: PackedStringArray = []
	lines.append(String(definition.get("summary", "")))
	var slots := int(definition.get("worker_slots", 0))
	if slots > 0:
		var workers := int(building.get("workers", 0))
		var status := "Working" if workers > 0 else "Idle: needs workers"
		lines.append("Workers: %d/%d (%s)" % [workers, slots, status])
	var housing := int(definition.get("housing", 0))
	if housing > 0:
		var residents := 0
		for citizen in ColonySimulation.citizens:
			if String(citizen.get("home_id", "")) == _info_id:
				residents += 1
		lines.append("Residents: %d/%d" % [residents, housing])
	_info_title.text = String(definition.get("label", building_type))
	_info_body.text = "\n".join(lines)
	_info_panel.visible = true


func _show_citizen_info() -> void:
	var info: Dictionary = ColonySimulation.get_citizen_inspection(_info_id)
	if info.is_empty():
		_hide_info()
		return
	var job := String(info.get("job", "Unassigned"))
	var lines: PackedStringArray = []
	lines.append("Age %d · %s" % [int(info.get("age_years", 0)), "Unemployed" if job == "Unassigned" else job])
	lines.append("Mood %d%%   Health %d%%" % [int(info.get("happiness", 0)), int(info.get("health", 0))])
	var needs: Dictionary = info.get("needs", {})
	var lowest_name := ""
	var lowest_value := 101.0
	for need_name in needs:
		if float(needs[need_name]) < lowest_value:
			lowest_value = float(needs[need_name])
			lowest_name = String(need_name)
	if not lowest_name.is_empty():
		lines.append("Lowest need: %s (%d%%)" % [lowest_name, int(lowest_value)])
	_info_title.text = String(info.get("name", "Colonist"))
	_info_body.text = "\n".join(lines)
	_info_panel.visible = true


# --- Menu and messages ----------------------------------------------------------------

func _open_menu() -> void:
	_menu_overlay.visible = true


func _close_menu() -> void:
	_menu_overlay.visible = false


func _on_menu_dim_input(event: InputEvent) -> void:
	if event is InputEventMouseButton and event.pressed:
		_close_menu()
	elif event is InputEventScreenTouch and event.pressed:
		_close_menu()


func _on_center_pressed() -> void:
	_world_view.focus_city()
	_close_menu()


func _on_save_pressed() -> void:
	var saved := SaveService.save_game(ColonySimulation.get_snapshot())
	_show_toast("Game saved." if saved else "Save failed. Check the log.")
	_close_menu()


func _on_load_pressed() -> void:
	var result: Dictionary = SaveService.load_game()
	if not bool(result.get("ok", false)):
		_show_toast(String(result.get("error", "Could not load save.")))
		_close_menu()
		return
	var payload: Dictionary = result.get("data", {})
	var colony_data: Dictionary = payload.get("colony", {})
	if not ColonySimulation.restore_snapshot(colony_data):
		_show_toast("Colony save data is invalid.")
		_close_menu()
		return
	_cancel_placement()
	_hide_info()
	_show_toast("Save loaded.")
	_close_menu()
	_refresh()


func _on_diagnostics_pressed() -> void:
	_close_menu()
	if is_instance_valid(_diagnostics_overlay):
		return
	_diagnostics_overlay = DiagnosticsScene.instantiate()
	_diagnostics_overlay.close_requested.connect(_close_diagnostics_overlay)
	add_child(_diagnostics_overlay)


func _close_diagnostics_overlay() -> void:
	if is_instance_valid(_diagnostics_overlay):
		_diagnostics_overlay.queue_free()
		_diagnostics_overlay = null


func _on_message_changed(message: String) -> void:
	# The routine daily summary would pop up every few seconds; only warnings matter.
	if message.begins_with("Day ") and not message.contains("shortage"):
		return
	_show_toast(message)


func _show_toast(message: String) -> void:
	if not is_instance_valid(_toast_label):
		return
	_toast_label.text = message
	_toast_panel.visible = true
	_toast_timer.start()


func _on_application_paused() -> void:
	_show_toast("Paused. Simulation time stops while the app is closed.")


func _on_application_resumed() -> void:
	_show_toast("Welcome back. Colony simulation resumed.")


# --- Refresh ------------------------------------------------------------------------------

func _refresh() -> void:
	if not is_instance_valid(_world_view):
		return
	var stock: Dictionary = ColonySimulation.resources
	for kind in _stock_labels:
		var label: Label = _stock_labels[kind]
		var amount := int(stock.get(kind, 0))
		label.text = str(amount)
		var low: bool = kind != "metal" and amount <= LOW_STOCK
		label.add_theme_color_override("font_color", GameTheme.BAD if low else GameTheme.TEXT)
	_people_label.text = str(ColonySimulation.citizens.size())
	_mood_label.text = "%d%%" % ColonySimulation.get_average_happiness()
	_day_label.text = "Day %d" % ColonySimulation.day

	_world_view.set_world(ColonySimulation.buildings, ColonySimulation.citizens, false, false, true, Vector2.ZERO)
	for building_type in _cards:
		_cards[building_type].set_affordable(ColonySimulation.can_build(String(building_type)))
	if not _placing_type.is_empty():
		_on_placement_changed(_world_view.is_ghost_valid())
	_update_info()
