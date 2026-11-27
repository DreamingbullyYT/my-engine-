extends Node

## Emits application lifecycle transitions without owning gameplay state.
signal application_paused
signal application_resumed

var is_application_paused: bool = false


func _notification(what: int) -> void:
	match what:
		NOTIFICATION_APPLICATION_PAUSED:
			is_application_paused = true
			application_paused.emit()
		NOTIFICATION_APPLICATION_RESUMED:
			is_application_paused = false
			application_resumed.emit()
