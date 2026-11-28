extends Button

## One tappable card in the build bar: live pixel preview, name and cost.
## It never changes colony state; it only reports that it was pressed.

const GameTheme = preload("res://ui/game_theme.gd")
const PixelPainterScript = preload("res://rendering/city/pixel_painter.gd")
const BuildingArtScript = preload("res://rendering/city/building_art.gd")
const PixelIconScript = preload("res://ui/widgets/pixel_icon.gd")

const CARD_SIZE := Vector2(116.0, 100.0)

var building_type: String = ""
var display_name: String = ""
var cost_amount: int = 0
var affordable: bool = true
var active: bool = false

var _painter = PixelPainterScript.new()


func _init() -> void:
	custom_minimum_size = CARD_SIZE
	focus_mode = Control.FOCUS_NONE
	clip_text = true


func setup(type_name: String, label: String, cost: int) -> void:
	building_type = type_name
	display_name = label
	cost_amount = cost
	queue_redraw()


func set_affordable(value: bool) -> void:
	if affordable == value:
		return
	affordable = value
	queue_redraw()


func set_active(value: bool) -> void:
	if active == value:
		return
	active = value
	queue_redraw()


func _draw() -> void:
	if active:
		draw_rect(Rect2(Vector2.ZERO, size), GameTheme.ACCENT, false, 3.0)

	var preview_height := 58.0
	var art_height := BuildingArtScript.height_of(building_type) + 10.0
	var scale := clampf((preview_height - 4.0) / art_height, 1.0, 2.0)
	var width_units := 36.0 if building_type != "water_pump" else 24.0
	var origin := Vector2(floorf((size.x - width_units * scale) * 0.5), preview_height)
	_painter.begin_flat(self, origin, scale)
	_painter.night = 0.0
	_painter.time = 0.0
	_painter.alpha = 1.0 if affordable else 0.45
	BuildingArtScript.paint(_painter, building_type, {
		"workers": 1, "slots": 0, "time": 0.0, "seed": 5, "width": width_units
	})

	var font := get_theme_default_font()
	var name_color := GameTheme.TEXT if affordable else GameTheme.TEXT_DIM
	draw_string(font, Vector2(6.0, preview_height + 18.0), display_name, HORIZONTAL_ALIGNMENT_CENTER, size.x - 12.0, 14, name_color)

	var cost_color := GameTheme.GOOD if affordable else GameTheme.BAD
	var cost_text := str(cost_amount)
	var text_width := font.get_string_size(cost_text, HORIZONTAL_ALIGNMENT_LEFT, -1.0, 14).x
	var total_width := 18.0 + text_width
	var start_x := floorf((size.x - total_width) * 0.5)
	_draw_metal_icon(Vector2(start_x, preview_height + 24.0))
	draw_string(font, Vector2(start_x + 20.0, preview_height + 36.0), cost_text, HORIZONTAL_ALIGNMENT_LEFT, -1.0, 14, cost_color)


func _draw_metal_icon(position_value: Vector2) -> void:
	var icon: Dictionary = PixelIconScript.ICONS["metal"]
	var rows: Array = icon["rows"]
	var colors: Dictionary = icon["colors"]
	for row_index in range(rows.size()):
		var row := String(rows[row_index])
		for column in range(row.length()):
			var key := row.substr(column, 1)
			if colors.has(key):
				draw_rect(Rect2(position_value + Vector2(float(column), float(row_index)) * 2.0, Vector2(2.0, 2.0)), colors[key])
