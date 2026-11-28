# DEPRECATED — D-023 rejects this planet-first overview as the main game view. The main scene uses rendering/world_view.gd instead. Retained temporarily as historical prototype code; do not wire it into app/main.tscn.
extends Control

const PLANET_RADIUS := 100.0
const MARKER_SIZE := 24.0

var _buildings: Array = []
var _rocket_state: int = 0
var _rocket_position: Vector2 = Vector2(0.0, -PLANET_RADIUS)


func set_world(buildings: Array, rocket_state: int, rocket_position: Vector2) -> void:
	_buildings = buildings.duplicate(true)
	_rocket_state = rocket_state
	_rocket_position = rocket_position
	queue_redraw()


func _draw() -> void:
	var centre := size * 0.5
	var radius := minf(size.x * 0.19, size.y * 0.34)
	radius = maxf(radius, 38.0)

	for index in range(22):
		var angle := float(index) * 2.399
		var distance := radius * (1.9 + float(index % 4) * 0.22)
		var star := centre + Vector2.RIGHT.rotated(angle) * distance
		draw_rect(Rect2(star, Vector2(2.0, 2.0)), Color(0.52, 0.66, 0.82, 0.75))

	draw_circle(centre, radius + 3.0, Color(0.15, 0.27, 0.36, 1.0))
	draw_circle(centre, radius, Color(0.24, 0.34, 0.39, 1.0))
	draw_arc(centre, radius, 0.0, TAU, 96, Color(0.62, 0.72, 0.65, 1.0), 2.0, false)
	draw_arc(centre, radius + 14.0, -PI, 0.0, 48, Color(0.33, 0.52, 0.61, 0.6), 1.0, false)

	for building in _buildings:
		var angle := float(building.get("angle", 0.0))
		var direction := Vector2.RIGHT.rotated(angle)
		var building_position := centre + direction * (radius + MARKER_SIZE * 0.48)
		_draw_building(building_position, String(building.get("type", "")))

	_draw_citizens(centre, radius)

	if _rocket_state > 0:
		var altitude := clampf(_rocket_position.length() - PLANET_RADIUS, 0.0, 90.0)
		var direction := _rocket_position.normalized()
		var marker_distance := radius + 28.0 + altitude * 0.20
		_draw_rocket(centre + direction * marker_distance)


func _draw_building(position: Vector2, building_type: String) -> void:
	var colour := Color(0.72, 0.72, 0.72, 1.0)
	var accent := Color(0.19, 0.22, 0.25, 1.0)
	match building_type:
		"habitat":
			colour = Color(0.72, 0.76, 0.83, 1.0)
			accent = Color(0.31, 0.48, 0.62, 1.0)
		"solar":
			colour = Color(0.27, 0.48, 0.75, 1.0)
			accent = Color(0.66, 0.81, 0.93, 1.0)
		"farm":
			colour = Color(0.37, 0.65, 0.38, 1.0)
			accent = Color(0.82, 0.77, 0.43, 1.0)
		"mine":
			colour = Color(0.56, 0.48, 0.38, 1.0)
			accent = Color(0.82, 0.68, 0.47, 1.0)
		"launchpad":
			colour = Color(0.69, 0.42, 0.34, 1.0)
			accent = Color(0.91, 0.76, 0.56, 1.0)

	draw_rect(Rect2(position - Vector2(12.0, 12.0), Vector2(24.0, 24.0)), Color(0.05, 0.08, 0.11, 1.0))
	draw_rect(Rect2(position - Vector2(10.0, 10.0), Vector2(20.0, 20.0)), colour)
	draw_rect(Rect2(position - Vector2(7.0, 6.0), Vector2(14.0, 5.0)), accent)
	draw_rect(Rect2(position - Vector2(3.0, 0.0), Vector2(6.0, 8.0)), Color(0.11, 0.14, 0.17, 1.0))


func _draw_citizens(centre: Vector2, radius: float) -> void:
	for index in range(20):
		var angle := -2.05 + float(index % 10) * 0.055
		var row := int(index / 10)
		var point := centre + Vector2.RIGHT.rotated(angle) * (radius + 2.0 + float(row) * 4.0)
		draw_rect(Rect2(point, Vector2(3.0, 3.0)), Color(0.93, 0.83, 0.63, 1.0))


func _draw_rocket(position: Vector2) -> void:
	draw_rect(Rect2(position - Vector2(5.0, 9.0), Vector2(10.0, 18.0)), Color(0.92, 0.88, 0.73, 1.0))
	draw_rect(Rect2(position - Vector2(2.0, 5.0), Vector2(4.0, 7.0)), Color(0.34, 0.62, 0.79, 1.0))
	draw_rect(Rect2(position - Vector2(3.0, -7.0), Vector2(6.0, 2.0)), Color(0.84, 0.43, 0.28, 1.0))
