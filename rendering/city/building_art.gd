extends RefCounted

## Code-drawn placeholder art for city buildings. Every building is drawn from
## blocky rectangles on a 12-unit tile grid so it can later be swapped for the
## user's 32x32 textures without touching placement or simulation.
## Presentation only: reads building facts, never mutates them.

const TILE := 12.0

const HEIGHTS: Dictionary = {
	"habitat": 46.0,
	"solar": 28.0,
	"farm": 33.0,
	"water_pump": 38.0,
	"mine": 40.0
}

const WALL := Color(0.76, 0.8, 0.86)
const WALL_SHADE := Color(0.6, 0.65, 0.74)
const TRIM := Color(0.36, 0.41, 0.52)
const CONCRETE := Color(0.46, 0.48, 0.52)
const CONCRETE_DARK := Color(0.33, 0.35, 0.4)
const GLASS := Color(0.52, 0.78, 0.92)
const GLASS_NIGHT := Color(0.1, 0.14, 0.26)
const WARM_LIGHT := Color(1.0, 0.82, 0.4, 0.95)
const FRAME := Color(0.24, 0.29, 0.37)
const STEEL := Color(0.52, 0.62, 0.72)
const STEEL_DARK := Color(0.36, 0.45, 0.56)
const LEAF := Color(0.3, 0.68, 0.32)
const LEAF_DARK := Color(0.2, 0.5, 0.26)
const SOIL := Color(0.4, 0.27, 0.17)
const WOOD := Color(0.5, 0.34, 0.2)
const WOOD_DARK := Color(0.36, 0.24, 0.15)
const ALERT := Color(0.93, 0.32, 0.27)


static func height_of(building_type: String) -> float:
	return float(HEIGHTS.get(building_type, 32.0))


## state: workers (int), slots (int), time (float), seed (int)
static func paint(p, building_type: String, state: Dictionary) -> void:
	match building_type:
		"habitat":
			_paint_habitat(p, state)
		"solar":
			_paint_solar(p, state)
		"farm":
			_paint_farm(p, state)
		"water_pump":
			_paint_pump(p, state)
		"mine":
			_paint_mine(p, state)
		_:
			p.rect(0.0, 0.0, TILE * 3.0, 24.0, WALL)
	_paint_status_bubble(p, building_type, state)


static func _paint_habitat(p, state: Dictionary) -> void:
	var seed_value := int(state.get("seed", 0))
	p.rect(0.0, 0.0, 36.0, 3.0, CONCRETE_DARK)
	p.rect(1.0, 3.0, 34.0, 38.0, WALL)
	p.rect(29.0, 3.0, 6.0, 38.0, WALL_SHADE)
	p.rect(0.0, 41.0, 36.0, 3.0, TRIM)
	p.rect(1.0, 23.0, 34.0, 1.0, WALL_SHADE)
	# Roof gear: antenna with a slow beacon and a small dish.
	p.rect(26.0, 44.0, 1.0, 9.0, TRIM)
	if int(float(state.get("time", 0.0)) * 1.2) % 2 == 0:
		p.rect(25.0, 53.0, 3.0, 2.0, ALERT)
		p.light(24.0, 52.0, 5.0, 4.0, Color(1.0, 0.3, 0.25, 0.7))
	p.rect(7.0, 44.0, 1.0, 4.0, TRIM)
	p.poly([Vector2(4.0, 48.0), Vector2(11.0, 48.0), Vector2(9.0, 51.0), Vector2(6.0, 51.0)], STEEL)
	# Door and windows.
	p.rect(14.0, 3.0, 8.0, 14.0, Color(0.31, 0.2, 0.14))
	p.rect(15.0, 3.0, 6.0, 12.0, Color(0.42, 0.28, 0.19))
	p.rect(20.0, 9.0, 1.0, 2.0, Color(0.9, 0.8, 0.4))
	_habitat_window(p, 4.0, 8.0, seed_value, 0, state)
	_habitat_window(p, 26.0, 8.0, seed_value, 1, state)
	_habitat_window(p, 4.0, 27.0, seed_value, 2, state)
	_habitat_window(p, 15.0, 27.0, seed_value, 3, state)
	_habitat_window(p, 26.0, 27.0, seed_value, 4, state)


static func _habitat_window(p, u: float, v: float, seed_value: int, index: int, state: Dictionary) -> void:
	var lit := _hash(seed_value * 7 + index) < 0.72
	var glass := GLASS.lerp(GLASS_NIGHT, clampf(p.night * 1.2, 0.0, 1.0))
	p.rect(u - 1.0, v - 1.0, 8.0, 11.0, FRAME)
	p.rect(u, v, 6.0, 9.0, glass)
	p.rect(u, v + 5.0, 6.0, 1.0, FRAME)
	p.rect(u + 3.0, v, 1.0, 9.0, FRAME)
	if p.night < 0.7:
		p.rect(u, v + 7.0, 2.0, 2.0, Color(1.0, 1.0, 1.0, 0.45))
	if lit:
		p.light(u, v, 6.0, 9.0, WARM_LIGHT)


static func _paint_solar(p, state: Dictionary) -> void:
	var working := int(state.get("workers", 0)) > 0
	p.rect(0.0, 0.0, 36.0, 2.0, CONCRETE_DARK)
	var panel := Color(0.17, 0.32, 0.62) if working else Color(0.18, 0.24, 0.36)
	var panel_light := Color(0.42, 0.65, 0.92) if working else Color(0.3, 0.38, 0.5)
	for index in range(3):
		var left := 2.0 + float(index) * 10.5
		p.rect(left + 4.0, 2.0, 2.0, 9.0, STEEL_DARK)
		p.poly([
			Vector2(left, 10.0), Vector2(left + 10.0, 14.0),
			Vector2(left + 10.0, 24.0), Vector2(left, 20.0)
		], panel)
		p.poly([
			Vector2(left + 0.5, 15.0), Vector2(left + 9.5, 18.6),
			Vector2(left + 9.5, 19.6), Vector2(left + 0.5, 16.0)
		], panel_light)
		p.poly([
			Vector2(left + 4.8, 11.9), Vector2(left + 5.8, 12.3),
			Vector2(left + 5.8, 22.3), Vector2(left + 4.8, 21.9)
		], panel_light)
	p.rect(30.0, 2.0, 6.0, 9.0, STEEL)
	p.rect(31.0, 3.0, 4.0, 5.0, STEEL_DARK)
	var led := Color(0.4, 0.95, 0.45) if working else ALERT
	p.rect(32.0, 8.0, 2.0, 2.0, led)
	p.light(31.0, 7.0, 4.0, 4.0, Color(led.r, led.g, led.b, 0.7))


static func _paint_farm(p, state: Dictionary) -> void:
	var time_value := float(state.get("time", 0.0))
	var working := int(state.get("workers", 0)) > 0
	p.rect(0.0, 0.0, 36.0, 4.0, SOIL)
	p.rect(0.0, 4.0, 2.0, 16.0, FRAME)
	p.rect(34.0, 4.0, 2.0, 16.0, FRAME)
	# Glass shell.
	p.rect(2.0, 4.0, 32.0, 16.0, Color(0.74, 0.92, 0.96, 0.5))
	p.poly([Vector2(0.0, 20.0), Vector2(36.0, 20.0), Vector2(18.0, 33.0)], Color(0.76, 0.94, 0.98, 0.55))
	p.poly([Vector2(3.0, 21.0), Vector2(33.0, 21.0), Vector2(18.0, 31.0)], Color(0.86, 0.98, 1.0, 0.3))
	p.rect(0.0, 19.0, 36.0, 2.0, FRAME)
	p.poly([Vector2(17.0, 21.0), Vector2(19.0, 21.0), Vector2(19.0, 33.0), Vector2(17.0, 33.0)], FRAME)
	# Crops with a gentle sway.
	for index in range(8):
		var left := 3.0 + float(index) * 4.0
		var sway := sin(time_value * 1.6 + float(index)) * 0.6
		var stalk := 5.0 + float(int(_hash(index + 11) * 5.0)) + (2.0 if working else 0.0)
		p.rect(left + 1.0, 4.0, 1.0, stalk, LEAF_DARK)
		p.rect(left + sway, 4.0 + stalk - 1.0, 3.0, 3.0, LEAF)
		if working and index % 3 == 0:
			p.rect(left + 1.0, 4.0 + stalk - 3.0, 1.0, 1.0, Color(0.92, 0.3, 0.25))
	p.light(4.0, 6.0, 28.0, 2.0, Color(1.0, 0.45, 0.85, 0.35))


static func _paint_pump(p, state: Dictionary) -> void:
	var time_value := float(state.get("time", 0.0))
	var working := int(state.get("workers", 0)) > 0
	p.rect(0.0, 0.0, 24.0, 3.0, CONCRETE_DARK)
	p.rect(3.0, 3.0, 2.0, 18.0, STEEL_DARK)
	p.rect(19.0, 3.0, 2.0, 18.0, STEEL_DARK)
	p.rect(4.0, 11.0, 16.0, 1.0, STEEL_DARK)
	p.rect(11.0, 3.0, 2.0, 18.0, Color(0.4, 0.5, 0.6))
	# Tank.
	p.rect(1.0, 21.0, 22.0, 13.0, STEEL)
	p.rect(1.0, 25.0, 22.0, 2.0, STEEL_DARK)
	p.rect(3.0, 23.0, 2.0, 9.0, Color(0.7, 0.8, 0.9))
	p.rect(19.0, 21.0, 4.0, 13.0, STEEL_DARK)
	p.rect(3.0, 34.0, 18.0, 2.0, STEEL_DARK)
	p.rect(11.0, 36.0, 2.0, 3.0, STEEL_DARK)
	# Outlet and falling water.
	p.rect(15.0, 14.0, 8.0, 2.0, STEEL_DARK)
	if working:
		for index in range(3):
			var phase := fmod(time_value * 1.4 + float(index) * 0.34, 1.0)
			p.rect(22.0, 13.0 - phase * 10.0, 1.0, 2.0, Color(0.45, 0.78, 0.95, 1.0 - phase * 0.4))
		p.rect(19.0, 3.0, 5.0, 1.0, Color(0.45, 0.78, 0.95, 0.8))
	else:
		p.rect(3.0, 21.0, 18.0, 1.0, Color(0.3, 0.35, 0.45))


static func _paint_mine(p, state: Dictionary) -> void:
	var time_value := float(state.get("time", 0.0))
	var working := int(state.get("workers", 0)) > 0
	p.rect(0.0, 0.0, 36.0, 3.0, Color(0.3, 0.28, 0.3))
	p.rect(0.0, 3.0, 6.0, 3.0, Color(0.38, 0.35, 0.36))
	# Shaft entrance and timber frame.
	p.rect(12.0, 3.0, 12.0, 13.0, Color(0.08, 0.07, 0.09))
	p.rect(11.0, 3.0, 2.0, 15.0, WOOD)
	p.rect(23.0, 3.0, 2.0, 15.0, WOOD)
	p.rect(10.0, 16.0, 16.0, 3.0, WOOD_DARK)
	# Headframe legs and wheel.
	p.poly([Vector2(8.0, 3.0), Vector2(11.0, 3.0), Vector2(18.0, 36.0), Vector2(15.0, 36.0)], WOOD)
	p.poly([Vector2(28.0, 3.0), Vector2(25.0, 3.0), Vector2(19.0, 36.0), Vector2(21.0, 36.0)], WOOD)
	p.rect(14.0, 24.0, 8.0, 2.0, WOOD_DARK)
	var wheel_center := Vector2(18.0, 36.0)
	var rotation := time_value * 3.2 if working else 0.0
	for index in range(4):
		var spoke := rotation + float(index) * PI * 0.5
		_line(p, wheel_center, wheel_center + Vector2(cos(spoke), sin(spoke)) * 5.0, 1.3, STEEL)
	p.rect(16.5, 34.5, 3.0, 3.0, STEEL_DARK)
	p.rect(17.5, 3.0, 1.0, 31.0, Color(0.15, 0.15, 0.18))
	# Ore cart.
	p.rect(27.0, 3.0, 8.0, 5.0, Color(0.42, 0.33, 0.26))
	p.rect(28.0, 8.0, 6.0, 2.0, Color(0.58, 0.6, 0.66))
	p.rect(28.0, 2.0, 2.0, 2.0, Color(0.1, 0.1, 0.12))
	p.rect(32.0, 2.0, 2.0, 2.0, Color(0.1, 0.1, 0.12))
	if working:
		for index in range(3):
			var phase := fmod(time_value * 0.7 + float(index) * 0.33, 1.0)
			var size := 3.0 + phase * 2.0
			p.rect(18.0 + float(index) * 3.0, 14.0 + phase * 16.0, size, size, Color(0.72, 0.68, 0.62, 0.5 * (1.0 - phase)))
		p.light(12.0, 3.0, 12.0, 4.0, Color(1.0, 0.7, 0.3, 0.45))


static func _paint_status_bubble(p, building_type: String, state: Dictionary) -> void:
	if int(state.get("slots", 0)) <= 0 or int(state.get("workers", 0)) > 0:
		return
	var width := float(state.get("width", 36.0))
	var bob := floorf(sin(float(state.get("time", 0.0)) * 3.0) * 1.5)
	var base := height_of(building_type) + 6.0 + bob
	var left := width * 0.5 - 5.0
	p.rect(left, base, 10.0, 11.0, Color(1.0, 1.0, 1.0, 0.95))
	p.rect(left + 3.0, base - 2.0, 4.0, 2.0, Color(1.0, 1.0, 1.0, 0.95))
	p.rect(left + 4.0, base + 4.0, 2.0, 5.0, ALERT)
	p.rect(left + 4.0, base + 2.0, 2.0, 1.5, ALERT)


static func _line(p, from: Vector2, to: Vector2, thickness: float, color: Color) -> void:
	var direction := (to - from).normalized()
	var normal := Vector2(-direction.y, direction.x) * thickness * 0.5
	p.poly([from + normal, to + normal, to - normal, from - normal], color)


static func _hash(value: int) -> float:
	return fposmod(sin(float(value) * 12.9898 + 78.233) * 43758.5453, 1.0)
