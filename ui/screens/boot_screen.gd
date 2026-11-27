extends Control

var _touch_count: int = 0

@onready var _engine_version: Label = %EngineVersion
@onready var _touch_count_label: Label = %TouchCount
@onready var _status_label: Label = %Status


func _ready() -> void:
	var version_info: Dictionary = Engine.get_version_info()
	_engine_version.text = "Godot %s · Android ARM64" % version_info.get("string", "unknown")
	_update_touch_count()

	AppLifecycle.application_paused.connect(_on_application_paused)
	AppLifecycle.application_resumed.connect(_on_application_resumed)


func _on_test_touch_pressed() -> void:
	_touch_count += 1
	_update_touch_count()
	_status_label.text = "Touch input received successfully."


func _on_application_paused() -> void:
	_status_label.text = "Application paused by Android."


func _on_application_resumed() -> void:
	_status_label.text = "Application resumed. UI state was retained."


func _update_touch_count() -> void:
	_touch_count_label.text = "Touch events: %d" % _touch_count
