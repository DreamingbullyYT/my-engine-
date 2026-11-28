extends Node

signal changed
signal message_changed(message: String)

enum FlightState {
	NOT_BUILT,
	ON_SURFACE,
	ASCENDING,
	ORBIT,
	RETURNING
}

const HomeWorldDefinition = preload("res://world/home_world_definition.gd")
const PLANET_RADIUS: float = HomeWorldDefinition.RADIUS
const TARGET_ORBIT_RADIUS := PLANET_RADIUS * 1.55
const GRAVITATIONAL_PARAMETER: float = HomeWorldDefinition.GRAVITATIONAL_PARAMETER
const ASCENT_ACCELERATION := 18.0
const MAX_FRAME_CATCHUP := 0.25
const PHYSICS_STEP := 0.05

var state: int = FlightState.NOT_BUILT
var position: Vector2 = Vector2(0.0, -PLANET_RADIUS)
var velocity: Vector2 = Vector2.ZERO
var flight_seconds: float = 0.0

var _display_accumulator: float = 0.0


func _process(delta: float) -> void:
	if state == FlightState.NOT_BUILT or state == FlightState.ON_SURFACE:
		return
	advance_simulation(delta)
	_display_accumulator += delta
	if _display_accumulator >= 0.15:
		_display_accumulator = 0.0
		changed.emit()


func construct() -> bool:
	if state != FlightState.NOT_BUILT:
		return false
	position = Vector2(0.0, -PLANET_RADIUS)
	velocity = Vector2.ZERO
	flight_seconds = 0.0
	state = FlightState.ON_SURFACE
	_emit_message("Rocket assembled at the launch pad.")
	changed.emit()
	return true


func can_launch() -> bool:
	return state == FlightState.ON_SURFACE


func can_return() -> bool:
	return state == FlightState.ORBIT


func is_unbuilt() -> bool:
	return state == FlightState.NOT_BUILT


func is_on_surface() -> bool:
	return state == FlightState.ON_SURFACE


func is_in_orbit() -> bool:
	return state == FlightState.ORBIT


func has_rocket() -> bool:
	return state != FlightState.NOT_BUILT


func is_flight_active() -> bool:
	return state == FlightState.ASCENDING or state == FlightState.ORBIT or state == FlightState.RETURNING


func launch() -> bool:
	if not can_launch():
		return false
	velocity = Vector2.ZERO
	flight_seconds = 0.0
	state = FlightState.ASCENDING
	_emit_message("Liftoff! Guidance is steering toward a stable circular orbit.")
	changed.emit()
	return true


func begin_return() -> bool:
	if not can_return():
		return false
	var radial := position.normalized()
	velocity = -radial * 8.0
	state = FlightState.RETURNING
	_emit_message("Deorbit burn started. The capsule is descending.")
	changed.emit()
	return true


func advance_simulation(delta_seconds: float) -> void:
	var remaining := minf(maxf(delta_seconds, 0.0), MAX_FRAME_CATCHUP)
	while remaining > 0.00001:
		var step := minf(PHYSICS_STEP, remaining)
		_step_physics(step)
		remaining -= step


func get_altitude() -> float:
	return maxf(0.0, position.length() - PLANET_RADIUS)


func get_orbital_speed() -> float:
	return velocity.length()


func get_circular_speed(radius: float) -> float:
	return sqrt(GRAVITATIONAL_PARAMETER / maxf(radius, 1.0))


func get_orbital_period_seconds() -> float:
	var radius := maxf(position.length(), PLANET_RADIUS)
	return TAU * sqrt(pow(radius, 3.0) / GRAVITATIONAL_PARAMETER)


func get_status_text() -> String:
	match state:
		FlightState.NOT_BUILT:
			return "Rocket: not built"
		FlightState.ON_SURFACE:
			return "Rocket: ready on the surface"
		FlightState.ASCENDING:
			return "Ascent · altitude %.1f units · speed %.1f u/s" % [get_altitude(), get_orbital_speed()]
		FlightState.ORBIT:
			return "Stable orbit · altitude %.1f units · speed %.1f u/s · period %.1f s" % [
				get_altitude(), get_orbital_speed(), get_orbital_period_seconds()
			]
		FlightState.RETURNING:
			return "Re-entry · altitude %.1f units · speed %.1f u/s" % [get_altitude(), get_orbital_speed()]
	return "Rocket status unavailable"


func get_snapshot() -> Dictionary:
	return {
		"schema": 1,
		"state": state,
		"position_x": position.x,
		"position_y": position.y,
		"velocity_x": velocity.x,
		"velocity_y": velocity.y,
		"flight_seconds": flight_seconds
	}


func restore_snapshot(data: Dictionary) -> bool:
	if int(data.get("schema", 0)) != 1:
		return false
	var incoming_state := int(data.get("state", FlightState.NOT_BUILT))
	if incoming_state < FlightState.NOT_BUILT or incoming_state > FlightState.RETURNING:
		return false
	state = incoming_state
	position = Vector2(
		float(data.get("position_x", 0.0)),
		float(data.get("position_y", -PLANET_RADIUS))
	)
	velocity = Vector2(
		float(data.get("velocity_x", 0.0)),
		float(data.get("velocity_y", 0.0))
	)
	flight_seconds = maxf(0.0, float(data.get("flight_seconds", 0.0)))
	if position.length() < PLANET_RADIUS:
		position = position.normalized() * PLANET_RADIUS
	changed.emit()
	return true


func _step_physics(delta: float) -> void:
	var radius := maxf(position.length(), PLANET_RADIUS)
	var radial := position.normalized()
	var gravity := -radial * (GRAVITATIONAL_PARAMETER / (radius * radius))

	match state:
		FlightState.ASCENDING:
			var thrust := radial * ASCENT_ACCELERATION
			velocity += (gravity + thrust) * delta
			position += velocity * delta
			flight_seconds += delta
			if position.length() >= TARGET_ORBIT_RADIUS:
				radial = position.normalized()
				var prograde := Vector2(-radial.y, radial.x)
				velocity = prograde * get_circular_speed(position.length())
				state = FlightState.ORBIT
				_emit_message("Orbit achieved. Circular velocity established.")
		FlightState.ORBIT:
			velocity += gravity * delta
			position += velocity * delta
			flight_seconds += delta
		FlightState.RETURNING:
			velocity += gravity * delta
			position += velocity * delta
			flight_seconds += delta
			if position.length() <= PLANET_RADIUS:
				position = position.normalized() * PLANET_RADIUS
				velocity = Vector2.ZERO
				state = FlightState.ON_SURFACE
				_emit_message("Landing complete. Rocket recovered at the surface.")


func _emit_message(message: String) -> void:
	message_changed.emit(message)
