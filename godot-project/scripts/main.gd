extends Control

var _status_label: Label
var _fps_label: Label
var _fps_refresh := 0.0
var _touch_count := 0

func _ready() -> void:
	_build_interface()

func _process(delta: float) -> void:
	_fps_refresh += delta
	if _fps_refresh >= 0.25:
		_fps_refresh = 0.0
		_fps_label.text = "Godot 4.7.2  |  %d FPS" % Engine.get_frames_per_second()

func _build_interface() -> void:
	set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	var background := ColorRect.new()
	background.color = Color("#101827")
	background.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	background.mouse_filter = Control.MOUSE_FILTER_IGNORE
	add_child(background)
	var center := CenterContainer.new()
	center.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	add_child(center)
	var panel := PanelContainer.new()
	panel.custom_minimum_size = Vector2(320, 250)
	var panel_style := StyleBoxFlat.new()
	panel_style.bg_color = Color("#1b2b40")
	panel_style.corner_radius_top_left = 18
	panel_style.corner_radius_top_right = 18
	panel_style.corner_radius_bottom_left = 18
	panel_style.corner_radius_bottom_right = 18
	panel_style.content_margin_left = 24
	panel_style.content_margin_right = 24
	panel_style.content_margin_top = 24
	panel_style.content_margin_bottom = 24
	panel.add_theme_stylebox_override("panel", panel_style)
	center.add_child(panel)
	var stack := VBoxContainer.new()
	stack.add_theme_constant_override("separation", 14)
	panel.add_child(stack)
	var title := Label.new()
	title.text = "DREAM ENGINE"
	title.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	title.add_theme_font_size_override("font_size", 28)
	title.add_theme_color_override("font_color", Color("#f3f6fb"))
	stack.add_child(title)
	var subtitle := Label.new()
	subtitle.text = "Powered by Godot 4.7.2"
	subtitle.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	subtitle.add_theme_font_size_override("font_size", 16)
	subtitle.add_theme_color_override("font_color", Color("#9fb3ca"))
	stack.add_child(subtitle)
	_status_label = Label.new()
	_status_label.text = "Android ARM64 runtime ready"
	_status_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	_status_label.add_theme_color_override("font_color", Color("#a8e6cf"))
	stack.add_child(_status_label)
	_fps_label = Label.new()
	_fps_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	_fps_label.add_theme_color_override("font_color", Color("#9fb3ca"))
	stack.add_child(_fps_label)
	var test_button := Button.new()
	test_button.text = "Test touch input"
	test_button.custom_minimum_size = Vector2(240, 52)
	test_button.size_flags_horizontal = Control.SIZE_SHRINK_CENTER
	test_button.pressed.connect(_on_test_pressed)
	stack.add_child(test_button)

func _on_test_pressed() -> void:
	_touch_count += 1
	_status_label.text = "Touch input OK  |  presses: %d" % _touch_count
