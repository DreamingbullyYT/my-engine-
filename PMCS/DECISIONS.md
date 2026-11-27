# Accepted Decisions

Change a decision only through a new entry explaining the reason, migration impact, and compatibility consequences.

## D-001 — Official unmodified Godot
Status: ACCEPTED. Use official Godot 4.7.x; do not maintain a custom source fork by default. Keep Dream code in a separate GDExtension. Reconsider source patches only if a required capability cannot be achieved through public extension points and alternatives are documented.

## D-002 — Separate Dream Runtime
Status: ACCEPTED. Dream runtime/API code lives outside Godot's source tree to keep upstream clean and upgrades manageable.

## D-003 — Android ARM64 first
Status: ACCEPTED. Primary ABI is arm64-v8a. CI must produce Android ARM64 artifacts before Android support is claimed.

## D-004 — GitHub Actions as primary build environment
Status: ACCEPTED. The user has no PC development environment. CI must be reproducible, produce useful logs/artifacts, and fail loudly. Physical-device validation is performed by the user.

## D-005 — Script-first .dmod
Status: ACCEPTED. Ordinary mods use Lua, JSON, scenes, and assets. Native extensions are optional advanced features, not a requirement for ordinary mod authors.

## D-006 — NeoForge-like extensibility, not compatibility
Status: ACCEPTED. Aim for registries, events, hooks, lifecycle, dependencies, networking, and data-driven content. Do not claim compatibility with NeoForge/Minecraft Java mods.

## D-007 — Native mods are trusted code
Status: ACCEPTED. Native code can crash, corrupt memory, access files, or compromise the process. Do not promise safety or sandboxing without a tested threat model.

## D-008 — Small modular changes
Status: ACCEPTED. Prefer separate modules and focused patches. Avoid invasive upstream changes.

## D-009 — PMCS is mandatory
Status: ACCEPTED. Update project memory in the same change as meaningful architecture, API, status, or workflow changes.
