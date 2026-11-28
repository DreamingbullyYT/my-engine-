extends Control

## Tiny 8x8 pixel icons drawn from text bitmaps, so the HUD needs no image files.
## Replace with the user's 32x32 textures later by swapping this widget's draw.

const ICONS: Dictionary = {
	"metal": {
		"rows": [
			"........",
			"..hhhh..",
			".hggggd.",
			".dddddd.",
			"hhhhhhhh",
			"gggggggd",
			"dddddddd",
			"........"
		],
		"colors": {"h": Color(0.88, 0.92, 0.97), "g": Color(0.64, 0.72, 0.82), "d": Color(0.4, 0.47, 0.58)}
	},
	"food": {
		"rows": [
			"....l...",
			"...ll...",
			".rrrlrr.",
			"rrhrrrrr",
			"rrrrrrrr",
			"rrrrrrrd",
			".rrrrrd.",
			"..rddd.."
		],
		"colors": {"l": Color(0.4, 0.78, 0.3), "r": Color(0.92, 0.28, 0.24), "h": Color(1.0, 0.7, 0.66), "d": Color(0.62, 0.16, 0.16)}
	},
	"water": {
		"rows": [
			"...b....",
			"...b....",
			"..bbb...",
			".bbhbb..",
			"bbhbbbb.",
			"bbbbbbb.",
			".bbbbbd.",
			"..ddd..."
		],
		"colors": {"b": Color(0.38, 0.7, 0.96), "h": Color(0.9, 0.97, 1.0), "d": Color(0.2, 0.42, 0.74)}
	},
	"energy": {
		"rows": [
			"....yy..",
			"...yy...",
			"..yyy...",
			".yyyyyy.",
			"...yyy..",
			"...yy...",
			"..yy....",
			"..y....."
		],
		"colors": {"y": Color(1.0, 0.85, 0.25)}
	},
	"people": {
		"rows": [
			"..ssss..",
			"..ssss..",
			"...ss...",
			".cccccc.",
			".cccccc.",
			".cc..cc.",
			".pp..pp.",
			"........"
		],
		"colors": {"s": Color(0.96, 0.8, 0.64), "c": Color(0.32, 0.58, 0.92), "p": Color(0.22, 0.27, 0.42)}
	},
	"happy": {
		"rows": [
			"..yyyy..",
			".yyyyyy.",
			"yykyykyy",
			"yyyyyyyy",
			"ykyyyyky",
			"yykkkkyy",
			".yyyyyy.",
			"..yyyy.."
		],
		"colors": {"y": Color(1.0, 0.84, 0.3), "k": Color(0.28, 0.2, 0.1)}
	},
	"menu": {
		"rows": [
			"........",
			".gggggg.",
			"........",
			".gggggg.",
			"........",
			".gggggg.",
			"........",
			"........"
		],
		"colors": {"g": Color(0.9, 0.93, 0.97)}
	},
	"close": {
		"rows": [
			"gg....gg",
			"ggg..ggg",
			".gggggg.",
			"..gggg..",
			"..gggg..",
			".gggggg.",
			"ggg..ggg",
			"gg....gg"
		],
		"colors": {"g": Color(0.9, 0.93, 0.97)}
	}
}

@export var kind: String = "metal":
	set(value):
		kind = value
		queue_redraw()

var tint: Color = Color(1.0, 1.0, 1.0, 1.0):
	set(value):
		tint = value
		queue_redraw()


func _init() -> void:
	custom_minimum_size = Vector2(24.0, 24.0)
	mouse_filter = Control.MOUSE_FILTER_IGNORE


func _draw() -> void:
	if not ICONS.has(kind):
		return
	var icon: Dictionary = ICONS[kind]
	var rows: Array = icon["rows"]
	var colors: Dictionary = icon["colors"]
	var scale := maxf(1.0, floorf(minf(size.x, size.y) / 8.0))
	var origin := (size - Vector2(8.0, 8.0) * scale) * 0.5
	origin = Vector2(floorf(origin.x), floorf(origin.y))
	for row_index in range(rows.size()):
		var row := String(rows[row_index])
		for column in range(row.length()):
			var key := row.substr(column, 1)
			if not colors.has(key):
				continue
			var color: Color = colors[key]
			draw_rect(Rect2(origin + Vector2(float(column), float(row_index)) * scale, Vector2(scale, scale)),
				Color(color.r * tint.r, color.g * tint.g, color.b * tint.b, color.a * tint.a))
