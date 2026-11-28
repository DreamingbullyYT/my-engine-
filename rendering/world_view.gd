extends Control

signal camera_scale_changed(scale_name: String)
signal camera_transition_finished

const HomeWorldDefinition = preload("res://world/home_world_definition.gd")
const PLANET_RADIUS: float = HomeWorldDefinition.RADIUS
const CITY_CENTER_ANGLE := -PI * 0.5
const CITY_START_ZOOM := 2.8
const PLANET_OVERVIEW_ZOOM := 0.38
const LOCKED_MIN_ZOOM := 1.45
const UNLOCKED_MIN_ZOOM := 0.32
const MAX_ZOOM := 5.0
const PLANET_REVEAL_THRESHOLD := 0.45
const CAMERA_SMOOTHING := 7.0
const REVEAL_DURATION := 3.2

var _buildings: Array = []
var _citizens: Array = []
var _rocket_exists: bool = false
var _rocket_position: Vector2 = Vector2(0.0, -PLANET_RADIUS)

var _launch_capability_unlocked: bool = false
var _camera_focus: Vector2 = Vector2(0.0, -PLANET_RADIUS)
var _target_camera_focus: Vector2 = Vector2(0.0, -PLANET_RADIUS)
var _camera_zoom: float = CITY_START_ZOOM
var _target_camera_zoom: float = CITY_START_ZOOM
var _transition_active: bool = false
var _transition_elapsed: float = 0.0
var _transition_start_zoom: float = CITY_START_ZOOM
var _transition_start_focus: Vector2 = Vector2(0.0, -PLANET_RADIUS)
var _touch_points: Dictionary = {}
var _last_pinch_distance: float = 0.0
var _mouse_pan_active: bool = false
var _last_mouse_position: Vector2 = Vector2.ZERO
var _last_scale_name: String = ""
var _last_rocket_in_flight: bool = false
var _follow_vehicle: bool = false


func _ready() -> void:
	mouse_filter = Control.MOUSE_FILTER_STOP
	clip_contents = true
	set_process(true)
	queue_redraw()


func set_world(
	buildings: Array,
	citizens: Array,
	rocket_exists: bool,
	rocket_in_flight: bool,
	rocket_on_surface: bool,
	rocket_position: Vector2
) -> void:
	_buildings = buildings.duplicate(true)
	_citizens = citizens.duplicate(true)
	_rocket_exists = rocket_exists
	_rocket_position = rocket_position

	if rocket_in_flight and not _last_rocket_in_flight:
		_follow_vehicle = true
		_target_camera_zoom = minf(_target_camera_zoom, PLANET_OVERVIEW_ZOOM)
	if rocket_in_flight and _follow_vehicle:
		_transition_active = false
		_target_camera_focus = rocket_position
	elif _last_rocket_in_flight and rocket_on_surface:
		focus_city()

	_last_rocket_in_flight = rocket_in_flight
	queue_redraw()


func set_launch_capability_unlocked(unlocked: bool, animate_reveal: bool = false) -> void:
	if not unlocked:
		_launch_capability_unlocked = false
		_transition_active = false
		_camera_focus = Vector2(0.0, -PLANET_RADIUS)
		_target_camera_focus = _camera_focus
		_camera_zoom = CITY_START_ZOOM
		_target_camera_zoom = CITY_START_ZOOM
		queue_redraw()
		return

	if _launch_capability_unlocked:
		return

	_launch_capability_unlocked = true
	if animate_reveal:
		_start_home_planet_reveal()
	else:
		_camera_focus = Vector2.ZERO
		_target_camera_focus = Vector2.ZERO
		_camera_zoom = PLANET_OVERVIEW_ZOOM
		_target_camera_zoom = PLANET_OVERVIEW_ZOOM
	queue_redraw()


func reveal_home_planet() -> void:
	set_launch_capability_unlocked(true, true)


func is_planetary_zoom_unlocked() -> bool:
	return _launch_capability_unlocked


func is_full_planet_visible() -> bool:
	return _launch_capability_unlocked and _camera_zoom <= PLANET_REVEAL_THRESHOLD


func get_camera_zoom() -> float:
	return _camera_zoom


func get_target_camera_zoom() -> float:
	return _target_camera_zoom


func get_target_camera_focus() -> Vector2:
	return _target_camera_focus


func is_camera_transition_active() -> bool:
	return _transition_active


func focus_city() -> void:
	_follow_vehicle = false
	_transition_active = false
	_target_camera_focus = Vector2(0.0, -PLANET_RADIUS)
	_target_camera_zoom = CITY_START_ZOOM
	_clamp_camera_focus()
	queue_redraw()


func get_camera_scale_name() -> String:
	if not _launch_capability_unlocked or _camera_zoom > 1.55:
		return "CITY"
	if _camera_zoom > PLANET_REVEAL_THRESHOLD:
		return "LOCAL SURFACE"
	return "HOME PLANET"


func _start_home_planet_reveal() -> void:
	if _transition_active:
		return
	_transition_start_zoom = _camera_zoom
	_transition_start_focus = _camera_focus
	_target_camera_zoom = PLANET_OVERVIEW_ZOOM
	_target_camera_focus = Vector2.ZERO
	_transition_elapsed = 0.0
	_transition_active = true


func _process(delta: float) -> void:
	var previous_zoom := _camera_zoom
	var previous_focus := _camera_focus

	if _transition_active:
		_transition_elapsed += delta
		var amount := clampf(_transition_elapsed / REVEAL_DURATION, 0.0, 1.0)
		var eased := amount * amount * (3.0 - 2.0 * amount)
		_camera_zoom = lerpf(_transition_start_zoom, _target_camera_zoom, eased)
		_camera_focus = _transition_start_focus.lerp(_target_camera_focus, eased)
		if amount >= 1.0:
			_transition_active = false
			camera_transition_finished.emit()
	else:
		var blend := 1.0 - exp(-CAMERA_SMOOTHING * delta)
		_camera_zoom = lerpf(_camera_zoom, _target_camera_zoom, blend)
		_camera_focus = _camera_focus.lerp(_target_camera_focus, blend)

	if not is_equal_approx(previous_zoom, _camera_zoom) or previous_focus.distance_to(_camera_focus) > 0.01:
		queue_redraw()

	var current_scale_name := get_camera_scale_name()
	if current_scale_name != _last_scale_name:
		_last_scale_name = current_scale_name
		camera_scale_changed.emit(current_scale_name)


func _gui_input(event: InputEvent) -> void:
	if event is InputEventScreenTouch:
		if event.pressed:
			_touch_points[event.index] = event.position
			if _touch_points.size() == 2:
				var points: Array = _touch_points.values()
				_last_pinch_distance = points[0].distance_to(points[1])
		else:
			_touch_points.erase(event.index)
			_last_pinch_distance = 0.0
		accept_event()
		return

	if event is InputEventScreenDrag:
		if not _touch_points.has(event.index):
			return
		if _touch_points.size() >= 2:
			_follow_vehicle = false
			var points: Array = _touch_points.keys()
			var other_id: int = int(points[0]) if int(points[1]) == event.index else int(points[1])
			var other_position: Vector2 = _touch_points[other_id]
			var new_distance: float = event.position.distance_to(other_position)
			if _last_pinch_distance > 0.0 and new_distance > 0.0:
				_apply_zoom(new_distance / _last_pinch_distance)
			_last_pinch_distance = new_distance
			_touch_points[event.index] = event.position
		else:
			_touch_points[event.index] = event.position
			_follow_vehicle = false
			_transition_active = false
			_target_camera_focus -= event.relative / maxf(_camera_zoom, 0.01)
			_clamp_camera_focus()
		accept_event()
		return

	if event is InputEventMouseButton:
		if event.button_index == MOUSE_BUTTON_LEFT:
			_mouse_pan_active = event.pressed
			_last_mouse_position = event.position
			accept_event()
		elif event.pressed and event.button_index == MOUSE_BUTTON_WHEEL_UP:
			_apply_zoom(1.12)
			accept_event()
		elif event.pressed and event.button_index == MOUSE_BUTTON_WHEEL_DOWN:
			_apply_zoom(0.89)
			accept_event()
		return

	if event is InputEventMouseMotion and _mouse_pan_active:
		_follow_vehicle = false
		_transition_active = false
		_target_camera_focus -= event.relative / maxf(_camera_zoom, 0.01)
		_last_mouse_position = event.position
		_clamp_camera_focus()
		accept_event()


func _apply_zoom(factor: float) -> void:
	_transition_active = false
	var minimum_zoom := UNLOCKED_MIN_ZOOM if _launch_capability_unlocked else LOCKED_MIN_ZOOM
	_target_camera_zoom = clampf(_target_camera_zoom * factor, minimum_zoom, MAX_ZOOM)
	_clamp_camera_focus()
	queue_redraw()


func _clamp_camera_focus() -> void:
	if not _launch_capability_unlocked:
		var city_origin := Vector2(0.0, -PLANET_RADIUS)
		var city_offset := _target_camera_focus - city_origin
		var maximum_local_pan := 180.0
		if city_offset.length() > maximum_local_pan:
			_target_camera_focus = city_origin + city_offset.normalized() * maximum_local_pan
	else:
		var maximum_planet_pan := PLANET_RADIUS * 1.7
		if _target_camera_focus.length() > maximum_planet_pan:
			_target_camera_focus = _target_camera_focus.normalized() * maximum_planet_pan


func _draw() -> void:
	draw_rect(Rect2(Vector2.ZERO, size), Color(0.43, 0.63, 0.75, 1.0))
	_draw_sky()

	var planet_mix := 0.0
	if _launch_capability_unlocked:
		planet_mix = clampf(inverse_lerp(1.35, PLANET_REVEAL_THRESHOLD, _camera_zoom), 0.0, 1.0)
	var screen_anchor := Vector2(size.x * 0.5, lerpf(size.y * 0.60, size.y * 0.5, planet_mix))
	var transform_position := screen_anchor - _camera_focus * _camera_zoom
	draw_set_transform(transform_position, 0.0, Vector2.ONE * _camera_zoom)

	_draw_city_surface(1.0 - planet_mix)
	_draw_home_planet(planet_mix)
	_draw_buildings()
	_draw_citizens()
	_draw_rocket_marker()

	draw_set_transform(Vector2.ZERO, 0.0, Vector2.ONE)
	_draw_view_label()


func _draw_sky() -> void:
	var star_visibility := 0.0
	if _launch_capability_unlocked:
		star_visibility = clampf(inverse_lerp(0.9, PLANET_REVEAL_THRESHOLD, _camera_zoom), 0.0, 1.0)

	var sky_visibility := 1.0 - star_visibility
	draw_circle(Vector2(size.x * 0.82, size.y * 0.32), 13.0, Color(0.96, 0.82, 0.53, sky_visibility))
	draw_rect(Rect2(Vector2(size.x * 0.12, size.y * 0.34), Vector2(30.0, 6.0)), Color(0.76, 0.85, 0.88, 0.5 * sky_visibility))
	draw_rect(Rect2(Vector2(size.x * 0.15, size.y * 0.31), Vector2(20.0, 8.0)), Color(0.76, 0.85, 0.88, 0.5 * sky_visibility))
	draw_rect(Rect2(Vector2(size.x * 0.66, size.y * 0.39), Vector2(24.0, 5.0)), Color(0.76, 0.85, 0.88, 0.42 * sky_visibility))

	if star_visibility > 0.001:
		for index in range(38):
			var angle := float(index) * 2.399
			var distance := 35.0 + float((index * 47) % 190)
			var point := size * 0.5 + Vector2.RIGHT.rotated(angle) * distance
			draw_rect(Rect2(point, Vector2(2.0, 2.0)), Color(0.84, 0.9, 0.96, 0.82 * star_visibility))


func _draw_city_surface(opacity: float) -> void:
	var visible_half_angle := clampf((size.x / maxf(_camera_zoom, 0.01)) / PLANET_RADIUS * 0.62, 0.36, 1.05)
	var start_angle := CITY_CENTER_ANGLE - visible_half_angle
	var end_angle := CITY_CENTER_ANGLE + visible_half_angle
	draw_arc(Vector2.ZERO, PLANET_RADIUS + 5.0, start_angle, end_angle, 96, Color(0.21, 0.25, 0.22, opacity), 24.0, true)
	draw_arc(Vector2.ZERO, PLANET_RADIUS - 7.0, start_angle, end_angle, 96, Color(0.36, 0.43, 0.29, opacity), 5.0, true)
	draw_arc(Vector2.ZERO, PLANET_RADIUS - 16.0, start_angle, end_angle, 96, Color(0.25, 0.31, 0.26, opacity), 2.0, true)
	draw_arc(Vector2.ZERO, PLANET_RADIUS + 20.0, start_angle, end_angle, 96, Color(0.24, 0.25, 0.22, opacity), 5.0, true)


func _draw_home_planet(opacity: float) -> void:
	if opacity <= 0.001:
		return
	draw_circle(Vector2.ZERO, PLANET_RADIUS, Color(0.18, 0.31, 0.25, opacity))
	draw_arc(Vector2.ZERO, PLANET_RADIUS, 0.0, TAU, 144, Color(0.73, 0.81, 0.7, opacity), 4.0, true)
	draw_arc(Vector2.ZERO, PLANET_RADIUS * 0.72, PI * 0.12, PI * 0.88, 80, Color(0.28, 0.43, 0.31, 0.8 * opacity), 3.0, true)
	draw_arc(Vector2.ZERO, PLANET_RADIUS * 0.84, PI * 1.1, PI * 1.9, 80, Color(0.27, 0.4, 0.3, 0.8 * opacity), 3.0, true)


func _draw_buildings() -> void:
	var aggregation := 0.0
	if _launch_capability_unlocked:
		aggregation = clampf(inverse_lerp(1.3, 0.78, _camera_zoom), 0.0, 1.0)

	for building in _buildings:
		var angle := float(building.get("angle", CITY_CENTER_ANGLE))
		var radial := Vector2.RIGHT.rotated(angle)
		var tangent := Vector2(-radial.y, radial.x)
		var base := radial * (PLANET_RADIUS + 8.0)
		var building_type := String(building.get("type", ""))

		if aggregation < 0.999:
			_draw_building(base, radial, tangent, building_type, 1.0 - aggregation)
		if aggregation > 0.001:
			var dot_color := _building_color(building_type)
			dot_color.a *= aggregation
			draw_rect(Rect2(base - Vector2(3.0, 3.0), Vector2(6.0, 6.0)), dot_color)


func _draw_building(base: Vector2, radial: Vector2, tangent: Vector2, building_type: String, opacity: float) -> void:
	var main_color := _building_color(building_type)
	main_color.a *= opacity
	var outline := Color(0.07, 0.1, 0.12, opacity)
	var half_width := 12.0
	var wall_height := 23.0

	var lower_left := base - tangent * half_width
	var lower_right := base + tangent * half_width
	var upper_left := lower_left + radial * wall_height
	var upper_right := lower_right + radial * wall_height
	draw_colored_polygon(PackedVector2Array([lower_left, lower_right, upper_right, upper_left]), main_color)
	draw_polyline(PackedVector2Array([lower_left, lower_right, upper_right, upper_left, lower_left]), outline, 2.0, true)

	var roof_peak := base + radial * 31.0
	var roof_left := upper_left + radial * 1.0
	var roof_right := upper_right + radial * 1.0
	draw_colored_polygon(PackedVector2Array([roof_left, roof_right, roof_peak]), Color(0.2, 0.25, 0.27, opacity))

	var window_color := Color(0.92, 0.8, 0.52, opacity)
	for side in [-1.0, 1.0]:
		var window_center: Vector2 = base + radial * 11.0 + tangent * side * 5.0
		var window_points := PackedVector2Array([
			window_center - tangent * 1.5 - radial * 2.0,
			window_center + tangent * 1.5 - radial * 2.0,
			window_center + tangent * 1.5 + radial * 2.0,
			window_center - tangent * 1.5 + radial * 2.0
		])
		draw_colored_polygon(window_points, window_color)


func _building_color(building_type: String) -> Color:
	match building_type:
		"habitat":
			return Color(0.69, 0.74, 0.79, 1.0)
		"solar":
			return Color(0.27, 0.49, 0.74, 1.0)
		"farm":
			return Color(0.36, 0.62, 0.36, 1.0)
		"mine":
			return Color(0.57, 0.48, 0.37, 1.0)
		"launchpad":
			return Color(0.71, 0.43, 0.34, 1.0)
	return Color(0.63, 0.66, 0.68, 1.0)


func _draw_citizens() -> void:
	var visibility := clampf(inverse_lerp(0.76, 1.05, _camera_zoom), 0.0, 1.0)
	if visibility <= 0.001:
		return
	var count := _citizens.size()
	if count <= 0:
		return
	for index in range(count):
		var offset := (float(index) - float(count - 1) * 0.5) * 5.6
		var angle := CITY_CENTER_ANGLE + offset / PLANET_RADIUS
		var radial := Vector2.RIGHT.rotated(angle)
		var tangent := Vector2(-radial.y, radial.x)
		var feet := radial * (PLANET_RADIUS + 9.0)
		var citizen_points := PackedVector2Array([
			feet - tangent * 2.0,
			feet + tangent * 2.0,
			feet + tangent * 2.0 + radial * 9.0,
			feet - tangent * 2.0 + radial * 9.0
		])
		var citizen_color := Color(0.93, 0.84, 0.66, visibility)
		draw_colored_polygon(citizen_points, citizen_color)


func _draw_rocket_marker() -> void:
	if not _rocket_exists:
		return
	var radial := _rocket_position.normalized()
	var tangent := Vector2(-radial.y, radial.x)
	var position := _rocket_position
	var half_width := 5.0
	var tip := position + radial * 15.0
	var tail_left := position - radial * 10.0 - tangent * half_width
	var tail_right := position - radial * 10.0 + tangent * half_width
	draw_colored_polygon(PackedVector2Array([tip, tail_right, tail_left]), Color(0.91, 0.9, 0.81, 1.0))


func _draw_view_label() -> void:
	var label := get_camera_scale_name()
	draw_string(ThemeDB.fallback_font, Vector2(18.0, 122.0), label, HORIZONTAL_ALIGNMENT_LEFT, -1.0, 16, Color(0.97, 0.98, 0.95, 0.92))
	if not _launch_capability_unlocked:
		draw_string(ThemeDB.fallback_font, Vector2(18.0, 144.0), "City phase · build and grow your settlement", HORIZONTAL_ALIGNMENT_LEFT, -1.0, 12, Color(0.94, 0.96, 0.93, 0.82))
