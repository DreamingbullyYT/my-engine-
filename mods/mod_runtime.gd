extends Node

signal probe_finished(success: bool, message: String)

const MODS_DIRECTORY := "user://mods"
const PROBE_PATH := "user://mods/_m3_external_probe.gd"
const PROBE_SOURCE := """extends Node

func execute(host: Node) -> void:
    host.call("report_external_mod_result", "M3_EXTERNAL_SCRIPT_OK")
"""


func run_external_probe() -> void:
    var directory_error := DirAccess.make_dir_recursive_absolute(
        ProjectSettings.globalize_path(MODS_DIRECTORY)
    )
    if directory_error != OK and directory_error != ERR_ALREADY_EXISTS:
        _fail("Cannot create user://mods/: %s" % error_string(directory_error))
        return

    var output := FileAccess.open(PROBE_PATH, FileAccess.WRITE)
    if output == null:
        _fail("Cannot write external script: %s" % error_string(FileAccess.get_open_error()))
        return
    output.store_string(PROBE_SOURCE)
    output.close()

    var input := FileAccess.open(PROBE_PATH, FileAccess.READ)
    if input == null:
        _fail("Cannot read external script: %s" % error_string(FileAccess.get_open_error()))
        return
    var external_source := input.get_as_text()
    input.close()

    var external_script := GDScript.new()
    external_script.source_code = external_source
    var parse_error: Error = external_script.reload()
    if parse_error != OK:
        _fail("External GDScript parse failed: %s" % error_string(parse_error))
        return

    var instance: Variant = external_script.new()
    if not instance is Node:
        _fail("External script did not create a Node instance.")
        return

    add_child(instance)
    instance.call("execute", self)
    instance.queue_free()


func report_external_mod_result(result: String) -> void:
    if result == "M3_EXTERNAL_SCRIPT_OK":
        print("[M3] EXTERNAL_GDSCRIPT_OK path=%s" % PROBE_PATH)
        probe_finished.emit(true, "External GDScript: OK · loaded from user://mods/")
    else:
        _fail("External script returned an unexpected result: %s" % result)


func _fail(message: String) -> void:
    push_error("[M3] %s" % message)
    probe_finished.emit(false, "External GDScript: FAILED · %s" % message)
