# Modding API Specification

Status: requirements/proposal only. No runtime implementation exists in the current branch.

## .dmod package
Proposed ZIP container. Required manifest fields:
- id: stable namespaced identifier.
- name: display name.
- version: semantic version.
- api_version: compatible Dream API range.
- entrypoint: script entry file.
- description, authors, license.

Optional fields:
- dependencies and version ranges.
- optional_dependencies.
- conflicts.
- load_after / load_before.
- capabilities requested from the host.
- resource namespaces.

Suggested contents: manifest.json, scripts/, data/, scenes/, assets/, LICENSE.

## API concepts

### Lifecycle
Provisional callbacks: on_load, on_enable, on_disable, on_unload, on_save, on_world_ready. Exact signatures remain undecided until implemented and tested.

### Event bus
Stable event IDs, typed payloads, subscribe/unsubscribe, deterministic priority/order, explicit cancellation semantics, and mod-attributed error handling.

### Registries
Namespaced IDs, duplicate detection, validation, registration phases, and extensible registry types for entities, items, blocks, components, recipes, effects, and game-defined content. Registries must not be a closed hardcoded list.

### Hooks
Before, Around, Replace, After. Stable target IDs, typed signatures, priority/order, owner identity, explicit replaceable targets, recursion protection, and diagnostics.

### Capabilities
Mods declare host capabilities they need. Potential families: world read/write, entity spawn/despawn, scene/resource registration, input, save-data namespaces, UI, physics, navigation, networking/session, audio, and rendering. Host games may deny or omit capabilities.

### Networking
Mod-defined namespaced messages, schema validation, explicit client/server authority, size limits, version negotiation, and entity/state sync helpers. Never assume a single-player game has a session/network integration.

### Mod-to-mod communication
Public service registration/lookup, typed messages/events, optional dependencies, and no default access to another mod's private globals.

## Entirely new gameplay systems
The API must allow a mod to implement systems absent from the original game, such as enemy AI, multiplayer, vehicles, crafting, quests, magic, or inventory. This requires composing exposed host primitives and registering mod-owned systems. Multiplayer additionally requires a host-approved authority/session/network path.

## Compatibility
Version engine, Dream Runtime, and Mod API independently. Use semantic versioning for public API, explicit manifest compatibility ranges, and a deprecation policy. Never silently reinterpret old manifest fields.

## Security
- Reject path traversal and unsafe ZIP extraction.
- Enforce package/resource size limits.
- Do not execute code during package validation.
- Attribute script errors to mod ID and source.
- Provide disable/quarantine controls.
- Lua VM isolation is not a security sandbox by itself.
- Native extensions are trusted code.
- Review current Android/store policies before enabling downloads of executable native code.

## Acceptance tests
1. Install and execute a script-only mod without APK rebuild.
2. Reject malformed manifests and archive path traversal.
3. Resolve dependencies; detect cycles and conflicts.
4. Register/query a custom registry entry.
5. Subscribe/unsubscribe to events.
6. Test all four hook stages on explicitly exposed targets.
7. Add an enemy and AI using documented script APIs.
8. Let two mods communicate through public contracts.
9. Reload a script mod with defined state semantics.
10. Connect two clients and sync a mod-owned entity in a host exposing networking.
