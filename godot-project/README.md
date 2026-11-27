# Dream Engine — Godot bootstrap

This minimal project validates the Godot 4.7.2-stable Android template compiled from the pinned source submodule. It targets arm64-v8a, uses the Godot Mobile renderer and displays a visible UI, FPS readout and touch-input button. It is not the final game or final engine UI.

The Actions workflow builds the Android native template from source, then uses the matching official Linux editor only to import and export this project. The APK native library is checked against the source-built output.
