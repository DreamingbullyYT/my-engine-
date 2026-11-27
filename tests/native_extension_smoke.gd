extends SceneTree

const PROBE_CLASS := "DreamRuntimeProbe"
const EXPECTED_MESSAGE := "dream-runtime-gdextension-ok"
const EXPECTED_SUM := 42


func _initialize() -> void:
    call_deferred("_run_native_probe")


func _run_native_probe() -> void:
    if not ClassDB.class_exists(PROBE_CLASS):
        push_error("[M2] DreamRuntimeProbe is not registered.")
        quit(1)
        return

    var probe: Object = ClassDB.instantiate(PROBE_CLASS)
    if probe == null:
        push_error("[M2] Godot could not instantiate DreamRuntimeProbe.")
        quit(1)
        return

    var message := String(probe.call("get_probe_message"))
    var result := int(probe.call("add_numbers", 20, 22))
    probe.free()

    if message != EXPECTED_MESSAGE:
        push_error("[M2] Unexpected native message: %s" % message)
        quit(1)
        return

    if result != EXPECTED_SUM:
        push_error("[M2] Native arithmetic returned %d, expected %d." % [result, EXPECTED_SUM])
        quit(1)
        return

    print("[M2] NATIVE_GDEXTENSION_SMOKE_OK: %s; 20 + 22 = %d" % [message, result])
    quit(0)
