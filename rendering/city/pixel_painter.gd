extends RefCounted

## Draws blocky "pixel" shapes in a local space where u runs along the ground
## and v runs up from it. In the normal side-on city view every shape is an
## exact axis-aligned rectangle. While the camera unrolls toward the planet the
## same shapes are bent onto the globe, so the city never needs a second
## renderer. Presentation only: it reads positions, it never changes them.

const CITY_CENTER_ANGLE := -PI * 0.5
const FLAT_THRESHOLD := 0.999

var canvas: CanvasItem
var world_radius: float = 600.0
var flat_mix: float = 1.0
var alpha: float = 1.0
var tint: Color = Color(1.0, 1.0, 1.0, 1.0)
var night: float = 0.0
var time: float = 0.0

var _exact_flat: bool = true
var _origin_angle: float = CITY_CENTER_ANGLE
var _flat_origin: Vector2 = Vector2.ZERO
var _flat_scale: float = 1.0
var _lights: Array = []


## City / planet drawing: the canvas transform already carries zoom and pan.
func begin_world(canvas_item: CanvasItem, radius: float, mix: float) -> void:
	canvas = canvas_item
	world_radius = radius
	flat_mix = mix
	_exact_flat = mix >= FLAT_THRESHOLD
	_flat_scale = 1.0
	_lights.clear()


## UI previews: draws in a plain flat space at `scale` canvas pixels per unit.
func begin_flat(canvas_item: CanvasItem, origin: Vector2, scale: float) -> void:
	canvas = canvas_item
	_exact_flat = true
	flat_mix = 1.0
	_flat_origin = origin
	_flat_scale = scale
	_lights.clear()


## Places local u = 0 at horizontal city position `x_units` (world units from
## the city centre, measured along the surface).
func set_origin_x(x_units: float) -> void:
	_origin_angle = CITY_CENTER_ANGLE + x_units / world_radius
	_flat_origin = Vector2(floorf(x_units), -world_radius)


func rect(u: float, v: float, w: float, h: float, color: Color) -> void:
	var shade := _shaded(color)
	if _exact_flat:
		canvas.draw_rect(Rect2(_flat_origin + Vector2(u, -v - h) * _flat_scale, Vector2(w, h) * _flat_scale), shade)
		return
	canvas.draw_colored_polygon(PackedVector2Array([
		_project(u, v), _project(u + w, v), _project(u + w, v + h), _project(u, v + h)
	]), shade)


## Filled polygon given as (u, v) points, used for roofs, panels and spokes.
func poly(points: Array, color: Color) -> void:
	var projected := PackedVector2Array()
	for point in points:
		projected.append(_project_point(point))
	canvas.draw_colored_polygon(projected, _shaded(color))


## Thin outline around a box, used for selection.
func outline(u: float, v: float, w: float, h: float, color: Color, thickness: float = 1.0) -> void:
	rect(u, v, w, thickness, color)
	rect(u, v + h - thickness, w, thickness, color)
	rect(u, v, thickness, h, color)
	rect(u + w - thickness, v, thickness, h, color)


## Marks a window or lamp that should glow after the night tint is applied.
func light(u: float, v: float, w: float, h: float, color: Color) -> void:
	if night < 0.25:
		return
	var glow := Color(color.r, color.g, color.b, color.a * clampf((night - 0.25) / 0.5, 0.0, 1.0) * alpha)
	if _exact_flat:
		_lights.append([Rect2(_flat_origin + Vector2(u, -v - h) * _flat_scale, Vector2(w, h) * _flat_scale), glow])
		return
	_lights.append([PackedVector2Array([
		_project(u, v), _project(u + w, v), _project(u + w, v + h), _project(u, v + h)
	]), glow])


func flush_lights() -> void:
	for entry in _lights:
		if entry[0] is Rect2:
			canvas.draw_rect(entry[0], entry[1])
		else:
			canvas.draw_colored_polygon(entry[0], entry[1])
	_lights.clear()


func _shaded(color: Color) -> Color:
	return Color(color.r * tint.r, color.g * tint.g, color.b * tint.b, color.a * tint.a * alpha)


func _project_point(point) -> Vector2:
	var local: Vector2 = point
	if _exact_flat:
		return _flat_origin + Vector2(local.x, -local.y) * _flat_scale
	return _project(local.x, local.y)


func _project(u: float, v: float) -> Vector2:
	var angle := _origin_angle + u / world_radius
	var polar := Vector2.RIGHT.rotated(angle) * (world_radius + v)
	var flat := Vector2((angle - CITY_CENTER_ANGLE) * world_radius, -world_radius - v)
	return polar.lerp(flat, flat_mix)
