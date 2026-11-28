extends RefCounted

## Sky, distant hills, clouds, ground and sparse vegetation for the side-on city
## view. Everything is deterministic from the camera position and a clock, so it
## needs no state and costs only a few hundred rectangles per frame.
## Presentation only.

const TILE := 12.0
const SKY_BANDS := 18

const DAY_TOP := Color(0.31, 0.56, 0.86)
const DAY_BOTTOM := Color(0.72, 0.88, 0.96)
const NIGHT_TOP := Color(0.02, 0.03, 0.11)
const NIGHT_BOTTOM := Color(0.1, 0.13, 0.28)
const DUSK_BOTTOM := Color(0.98, 0.6, 0.36)
const DUSK_TOP := Color(0.4, 0.37, 0.64)

const GRASS_LIGHT := Color(0.52, 0.8, 0.33)
const GRASS := Color(0.37, 0.68, 0.27)
const DIRT_A := Color(0.52, 0.36, 0.22)
const DIRT_B := Color(0.42, 0.29, 0.19)
const STONE := Color(0.34, 0.33, 0.37)
const STONE_DEEP := Color(0.26, 0.25, 0.3)


## Clock runs 0..1: 0 = midnight, 0.25 = sunrise, 0.5 = noon, 0.75 = sunset.
static func sun_height(clock: float) -> float:
	return -cos(clock * TAU)


static func daylight(clock: float) -> float:
	return smoothstep(-0.18, 0.22, sun_height(clock))


static func night_amount(clock: float) -> float:
	return 1.0 - daylight(clock)


static func draw_sky(canvas: CanvasItem, size: Vector2, horizon_y: float, clock: float, time: float, camera_shift: float) -> void:
	var height_now := sun_height(clock)
	var light := daylight(clock)
	var twilight := clampf(1.0 - absf(height_now) * 3.5, 0.0, 1.0) * 0.85
	var top := NIGHT_TOP.lerp(DAY_TOP, light).lerp(DUSK_TOP, twilight * 0.5)
	var bottom := NIGHT_BOTTOM.lerp(DAY_BOTTOM, light).lerp(DUSK_BOTTOM, twilight)
	var sky_bottom := maxf(horizon_y, 1.0)
	var band_height := sky_bottom / float(SKY_BANDS)
	for index in range(SKY_BANDS):
		var amount := float(index) / float(SKY_BANDS - 1)
		canvas.draw_rect(
			Rect2(0.0, floorf(float(index) * band_height), size.x, ceilf(band_height) + 1.0),
			top.lerp(bottom, amount * amount * 0.35 + amount * 0.65))
	if horizon_y < size.y:
		canvas.draw_rect(Rect2(0.0, horizon_y, size.x, size.y - horizon_y), bottom)

	_draw_stars(canvas, size, horizon_y, time, 1.0 - light)
	_draw_sun_and_moon(canvas, size, horizon_y, clock)
	_draw_hills(canvas, size, horizon_y, light, twilight, camera_shift)
	_draw_clouds(canvas, size, horizon_y, light, twilight, time, camera_shift)


static func _draw_stars(canvas: CanvasItem, size: Vector2, horizon_y: float, time: float, night: float) -> void:
	if night < 0.05:
		return
	for index in range(46):
		var x := _hash(index * 3 + 1) * size.x
		var y := _hash(index * 3 + 2) * horizon_y * 0.8
		var twinkle := 0.55 + 0.45 * sin(time * (0.8 + _hash(index) * 1.6) + float(index))
		canvas.draw_rect(Rect2(floorf(x), floorf(y), 2.0, 2.0), Color(0.88, 0.92, 1.0, night * twinkle))


static func _draw_sun_and_moon(canvas: CanvasItem, size: Vector2, horizon_y: float, clock: float) -> void:
	var phase := (clock - 0.25) * TAU
	var arc_height := horizon_y * 0.82
	var sun_pos := Vector2(size.x * (0.5 - 0.45 * cos(phase)), horizon_y - sin(phase) * arc_height)
	if sin(phase) > -0.08:
		canvas.draw_circle(sun_pos, 26.0, Color(1.0, 0.9, 0.6, 0.18))
		canvas.draw_circle(sun_pos, 18.0, Color(1.0, 0.92, 0.62, 0.35))
		canvas.draw_circle(sun_pos, 12.0, Color(1.0, 0.96, 0.72, 1.0))
	var moon_phase := phase + PI
	var moon_pos := Vector2(size.x * (0.5 - 0.45 * cos(moon_phase)), horizon_y - sin(moon_phase) * arc_height)
	if sin(moon_phase) > -0.08:
		canvas.draw_circle(moon_pos, 16.0, Color(0.8, 0.86, 1.0, 0.12))
		canvas.draw_circle(moon_pos, 10.0, Color(0.9, 0.93, 0.98, 1.0))
		canvas.draw_circle(moon_pos + Vector2(-3.0, -2.0), 2.4, Color(0.74, 0.78, 0.86, 1.0))
		canvas.draw_circle(moon_pos + Vector2(3.0, 3.0), 1.8, Color(0.76, 0.8, 0.88, 1.0))


static func _draw_hills(canvas: CanvasItem, size: Vector2, horizon_y: float, light: float, twilight: float, camera_shift: float) -> void:
	var far_color := Color(0.09, 0.12, 0.24).lerp(Color(0.56, 0.7, 0.84), light).lerp(Color(0.62, 0.5, 0.62), twilight * 0.5)
	var near_color := Color(0.06, 0.09, 0.18).lerp(Color(0.4, 0.58, 0.62), light).lerp(Color(0.46, 0.38, 0.5), twilight * 0.5)
	_draw_ridge(canvas, size, horizon_y, camera_shift * 0.12, 0.7, 92.0, far_color)
	_draw_ridge(canvas, size, horizon_y, camera_shift * 0.28, 3.1, 58.0, near_color)


static func _draw_ridge(canvas: CanvasItem, size: Vector2, horizon_y: float, scroll: float, phase: float, amplitude: float, color: Color) -> void:
	var column_width := 8.0
	var columns := int(ceil(size.x / column_width)) + 1
	for column in range(columns):
		var x := float(column) * column_width
		var world_x := x - scroll
		var wave := 0.5 + 0.5 * (
			sin(world_x * 0.011 + phase) * 0.55 +
			sin(world_x * 0.027 + phase * 1.7) * 0.3 +
			sin(world_x * 0.061 + phase * 2.3) * 0.15)
		var height := floorf(wave * amplitude / 6.0) * 6.0 + 6.0
		canvas.draw_rect(Rect2(x, horizon_y - height, column_width + 1.0, height + 2.0), color)


static func _draw_clouds(canvas: CanvasItem, size: Vector2, horizon_y: float, light: float, twilight: float, time: float, camera_shift: float) -> void:
	var cloud_color := Color(0.12, 0.15, 0.28).lerp(Color(1.0, 1.0, 1.0), light).lerp(Color(1.0, 0.8, 0.74), twilight * 0.6)
	cloud_color.a = 0.35 + 0.5 * light
	var span := size.x + 260.0
	for index in range(6):
		var speed := 3.0 + _hash(index + 40) * 5.0
		var base_x := _hash(index + 50) * span
		var x := fposmod(base_x + time * speed - camera_shift * 0.05, span) - 130.0
		var y := floorf(horizon_y * (0.1 + 0.42 * _hash(index + 60)) / 4.0) * 4.0
		var unit := 6.0 + floorf(_hash(index + 70) * 3.0) * 2.0
		canvas.draw_rect(Rect2(floorf(x) + unit * 2.0, y, unit * 4.0, unit), cloud_color)
		canvas.draw_rect(Rect2(floorf(x), y + unit, unit * 8.0, unit), cloud_color)
		canvas.draw_rect(Rect2(floorf(x) + unit, y + unit * 2.0, unit * 6.0, unit), cloud_color)


## Ground, grass tufts, sparse plants and mineral flecks between x0 and x1
## (world units from the city centre). `p` is a PixelPainter already prepared
## with begin_world(); `intervals` are occupied building tiles.
static func draw_ground(p, x0: float, x1: float, intervals: Array) -> void:
	var left := floorf(x0 / TILE) * TILE
	var right := ceilf(x1 / TILE) * TILE
	p.set_origin_x(left)
	var span := right - left
	_ground_layer(p, span, 0.0, -1.0, GRASS_LIGHT)
	_ground_layer(p, span, -1.0, -4.0, GRASS)
	_ground_layer(p, span, -4.0, -36.0, DIRT_A)
	_ground_layer(p, span, -36.0, -96.0, DIRT_B)
	_ground_layer(p, span, -96.0, -220.0, STONE)
	_ground_layer(p, span, -220.0, -520.0, STONE_DEEP)

	var first_tile := int(floor(left / TILE))
	var last_tile := int(ceil(right / TILE))
	for tile in range(first_tile, last_tile):
		var u := float(tile) * TILE - left
		_ground_details(p, tile, u)
		if _tile_is_free(float(tile), intervals):
			_ground_plants(p, tile, u)


static func _ground_layer(p, span: float, top: float, bottom: float, color: Color) -> void:
	var segment := span if p.flat_mix >= 0.999 else 36.0
	var u := 0.0
	while u < span - 0.01:
		var width := minf(segment, span - u)
		p.rect(u, bottom, width, top - bottom, color)
		u += width


static func _ground_details(p, tile: int, u: float) -> void:
	for index in range(5):
		var seed_a := _hash(tile * 13 + index)
		var seed_b := _hash(tile * 29 + index * 7 + 3)
		var speck_u := u + floorf(seed_a * 11.0)
		var depth := 5.0 + floorf(seed_b * 28.0)
		var color := DIRT_B if index % 2 == 0 else Color(0.6, 0.43, 0.27)
		p.rect(speck_u, -depth, 2.0, 1.0, color)
	for index in range(4):
		var depth := 40.0 + floorf(_hash(tile * 5 + index + 100) * 52.0)
		p.rect(u + floorf(_hash(tile * 11 + index) * 10.0), -depth, 2.0, 2.0, Color(0.5, 0.36, 0.24))
	if _hash(tile * 7 + 5) < 0.55:
		var ore_depth := 100.0 + floorf(_hash(tile * 3 + 9) * 110.0)
		var ore_color := Color(0.58, 0.66, 0.8) if _hash(tile + 4) < 0.6 else Color(0.8, 0.52, 0.3)
		p.rect(u + floorf(_hash(tile * 17 + 2) * 9.0), -ore_depth, 3.0, 2.0, ore_color)
	if _hash(tile * 3 + 1) < 0.5:
		p.rect(u + floorf(_hash(tile * 19) * 10.0), 0.0, 1.0, 2.0, GRASS_LIGHT)
		p.rect(u + floorf(_hash(tile * 23) * 10.0), 0.0, 1.0, 1.0, GRASS)


static func _ground_plants(p, tile: int, u: float) -> void:
	var roll := _hash(tile * 5 + 1)
	if roll < 0.07:
		var tall := floorf(_hash(tile + 200) * 3.0)
		p.rect(u + 5.0, 0.0, 2.0, 5.0 + tall, Color(0.4, 0.27, 0.16))
		p.rect(u + 2.0, 5.0 + tall, 8.0, 4.0, Color(0.2, 0.5, 0.28))
		p.rect(u + 3.0, 9.0 + tall, 6.0, 4.0, Color(0.24, 0.58, 0.31))
		p.rect(u + 5.0, 13.0 + tall, 2.0, 3.0, Color(0.3, 0.66, 0.34))
	elif roll < 0.17:
		p.rect(u + 3.0, 0.0, 6.0, 3.0, Color(0.27, 0.58, 0.28))
		p.rect(u + 4.0, 3.0, 4.0, 1.0, Color(0.4, 0.72, 0.34))
	elif roll < 0.22:
		p.rect(u + 4.0, 0.0, 5.0, 2.0, Color(0.5, 0.5, 0.54))
		p.rect(u + 5.0, 2.0, 3.0, 1.0, Color(0.62, 0.62, 0.66))


static func _tile_is_free(tile: float, intervals: Array) -> bool:
	for interval in intervals:
		var other: Vector2 = interval
		if tile + 1.0 > other.x - 1.0 and tile < other.y + 1.0:
			return false
	return true


static func _hash(value: int) -> float:
	return fposmod(sin(float(value) * 12.9898 + 78.233) * 43758.5453, 1.0)
