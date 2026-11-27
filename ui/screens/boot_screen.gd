extends Control

var _touch_count: int = 0

@onready var _engine_version: Label = %EngineVersion
@onready var _native_status: Label = %NativeStatus
@onready var _touch_count_label: Label = %TouchCount
@onready var _status_label: Label = %Status


func _ready() -> void:
    var version_info: Dictionary = Engine.get_version_info()
    _engine_version.text = "Godot %s · Android ARM64" % version_info.get("string", "unknown")
    _update_touch_count()
    _check_native_runtime()

    AppLifecycle.application_paused.connect(_on_application_paused)
    AppLifecycle.application_resumed.connect(_on_application_resumed)


func _check_native_runtime() -> void:
    const PROBE_CLASS := "DreamRuntimeProbe"

    if not ClassDB.class_exists(PROBE_CLASS):
        _native_status.text = "Dream Runtime: not loaded"
        push_error("[M2] DreamRuntimeProbe is not registered.")
        return

    var probe: Object = ClassDB.instantiate(PROBE_CLASS)
    if probe == null:
        _native_status.text = "Dream Runtime: instantiation failed"
        push_error("[M2] Godot could not instantiate DreamRuntimeProbe.")
        return

    var message := String(probe.call("get_probe_message"))
    var result := int(probe.call("add_numbers", 20, 22))
    probe.free()

    if message == "dream-runtime-gdextension-ok" and result == 42:
        _native_status.text = "Native C++: OK · 20 + 22 = 42"
        _status_label.text = "Dream Runtime GDExtension loaded successfully."
        print("[M2] ANDROID_NATIVE_PROBE_OK")
    else:
        _native_status.text = "Dream Runtime: test failed"
        push_error("[M2] Native probe returned an unexpected result.")


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
