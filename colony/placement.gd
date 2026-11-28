extends RefCounted

## Pure placement geometry for the radial city strip.
## Tiles are measured from the city centre along the surface. One tile spans
## TILE_ANGLE radians, so a tile is 12 world units on the 600-unit home world.
## This module holds no state and never touches scenes, so simulation, rendering
## and tests can share the exact same footprint rules.

const CITY_CENTER_ANGLE := -PI * 0.5
const TILE_ANGLE := 0.02
const MAX_TILE_OFFSET := 44.0
const EDGE_EPSILON := 0.001
const MAGNET_TILES := 1.01


static func angle_to_tile(angle: float) -> float:
	return (angle - CITY_CENTER_ANGLE) / TILE_ANGLE


static func tile_to_angle(tile: float) -> float:
	return CITY_CENTER_ANGLE + tile * TILE_ANGLE


## Interval [left, right) in tile units for a building centred at `angle`.
static func interval_for(angle: float, width_tiles: float) -> Vector2:
	var left := angle_to_tile(angle) - width_tiles * 0.5
	return Vector2(left, left + width_tiles)


static func is_free(center_tile: float, width_tiles: float, intervals: Array) -> bool:
	var left := center_tile - width_tiles * 0.5
	var right := left + width_tiles
	if left < -MAX_TILE_OFFSET - EDGE_EPSILON or right > MAX_TILE_OFFSET + EDGE_EPSILON:
		return false
	for interval in intervals:
		var other: Vector2 = interval
		if left < other.y - EDGE_EPSILON and right > other.x + EDGE_EPSILON:
			return false
	return true


## Snaps a requested centre tile to the whole-tile grid and magnets it flush
## against a neighbouring building when it is within one tile of an edge.
static func snap_center_tile(target_tile: float, width_tiles: float, intervals: Array) -> float:
	var left := roundf(target_tile - width_tiles * 0.5)
	var best := left
	var best_distance := MAGNET_TILES
	for interval in intervals:
		var other: Vector2 = interval
		for candidate in [other.y, other.x - width_tiles]:
			var distance := absf(float(candidate) - left)
			if distance < best_distance:
				best_distance = distance
				best = float(candidate)
	best = clampf(best, -MAX_TILE_OFFSET, MAX_TILE_OFFSET - width_tiles)
	return best + width_tiles * 0.5


## Nearest free centre to `target_tile` in half-tile steps, or NAN if the strip
## has no room left for a building of this width.
static func nearest_free_center(target_tile: float, width_tiles: float, intervals: Array) -> float:
	var best := NAN
	var best_distance := INF
	var steps := int(MAX_TILE_OFFSET * 4.0)
	for index in range(-steps, steps + 1):
		var left := float(index) * 0.5
		var center := left + width_tiles * 0.5
		var distance := absf(center - target_tile)
		if distance >= best_distance:
			continue
		if is_free(center, width_tiles, intervals):
			best = center
			best_distance = distance
	return best
